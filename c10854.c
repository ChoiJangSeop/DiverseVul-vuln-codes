int __dns_lookup(const char *name,
		int type,
		unsigned char **outpacket,
		struct resolv_answer *a)
{
	/* Protected by __resolv_lock: */
	static int last_ns_num = 0;
	static uint16_t last_id = 1;

	int i, j, fd, rc;
	int packet_len;
	int name_len;
#ifdef USE_SELECT
	struct timeval tv;
	fd_set fds;
#else
	struct pollfd fds;
#endif
	struct resolv_header h;
	struct resolv_question q;
	struct resolv_answer ma;
	bool first_answer = 1;
	int retries_left;
	unsigned char *packet = malloc(PACKETSZ);
	char *lookup;
	int variant = -1;  /* search domain to append, -1: none */
	int local_ns_num = -1; /* Nth server to use */
	int local_id = local_id; /* for compiler */
	int sdomains = 0;
	bool ends_with_dot;
	bool contains_dot;
	sockaddr46_t sa;

	fd = -1;
	lookup = NULL;
	name_len = strlen(name);
	if ((unsigned)name_len >= MAXDNAME - MAXLEN_searchdomain - 2)
		goto fail; /* paranoia */
	lookup = malloc(name_len + 1/*for '.'*/ + MAXLEN_searchdomain + 1);
	if (!packet || !lookup || !name[0])
		goto fail;
	ends_with_dot = (name[name_len - 1] == '.');
	contains_dot = strchr(name, '.') != NULL;
	/* no strcpy! paranoia, user might change name[] under us */
	memcpy(lookup, name, name_len);

	DPRINTF("Looking up type %d answer for '%s'\n", type, name);
	retries_left = 0; /* for compiler */
	do {
		unsigned act_variant;
		int pos;
		unsigned reply_timeout;

		if (fd != -1) {
			close(fd);
			fd = -1;
		}

		/* Mess with globals while under lock */
		/* NB: even data *pointed to* by globals may vanish
		 * outside the locks. We should assume any and all
		 * globals can completely change between locked
		 * code regions. OTOH, this is rare, so we don't need
		 * to handle it "nicely" (do not skip servers,
		 * search domains, etc), we only need to ensure
		 * we do not SEGV, use freed+overwritten data
		 * or do other Really Bad Things. */
		__UCLIBC_MUTEX_LOCK(__resolv_lock);
		__open_nameservers();
		if (type != T_PTR) {
			sdomains = __searchdomains;
		}
		lookup[name_len] = '\0';
		/* For qualified names, act_variant = MAX_UINT, 0, .., sdomains-1
		 *  => Try original name first, then append search domains
		 * For names without domain, act_variant = 0, 1, .., sdomains
		 *  => Try search domains first, original name last */
		act_variant = contains_dot ? variant : variant + 1;
		if (act_variant < sdomains) {
			/* lookup is name_len + 1 + MAXLEN_searchdomain + 1 long */
			/* __searchdomain[] is not bigger than MAXLEN_searchdomain */
			lookup[name_len] = '.';
			strcpy(&lookup[name_len + 1], __searchdomain[act_variant]);
		}
		/* first time? pick starting server etc */
		if (local_ns_num < 0) {
			local_id = last_id;
/*TODO: implement /etc/resolv.conf's "options rotate"
 (a.k.a. RES_ROTATE bit in _res.options)
			local_ns_num = 0;
			if (_res.options & RES_ROTATE) */
				local_ns_num = last_ns_num;
			retries_left = __nameservers * __resolv_attempts;
		}
		if (local_ns_num >= __nameservers)
			local_ns_num = 0;
		local_id++;
		local_id &= 0xffff;
		/* write new values back while still under lock */
		last_id = local_id;
		last_ns_num = local_ns_num;
		/* struct copy */
		/* can't just take a pointer, __nameserver[x]
		 * is not safe to use outside of locks */
		sa = __nameserver[local_ns_num];
		__UCLIBC_MUTEX_UNLOCK(__resolv_lock);

		memset(packet, 0, PACKETSZ);
		memset(&h, 0, sizeof(h));

		/* encode header */
		h.id = local_id;
		h.qdcount = 1;
		h.rd = 1;
		DPRINTF("encoding header\n", h.rd);
		i = __encode_header(&h, packet, PACKETSZ);
		if (i < 0)
			goto fail;

		/* encode question */
		DPRINTF("lookup name: %s\n", lookup);
		q.dotted = lookup;
		q.qtype = type;
		q.qclass = C_IN; /* CLASS_IN */
		j = __encode_question(&q, packet+i, PACKETSZ-i);
		if (j < 0)
			goto fail;
		packet_len = i + j;

		/* send packet */
#ifdef DEBUG
		{
			const socklen_t plen = sa.sa.sa_family == AF_INET ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN;
			char *pbuf = malloc(plen);
			if (pbuf == NULL) ;/* nothing */
#ifdef __UCLIBC_HAS_IPV6__
			else if (sa.sa.sa_family == AF_INET6)
				pbuf = (char*)inet_ntop(AF_INET6, &sa.sa6.sin6_addr, pbuf, plen);
#endif
#ifdef __UCLIBC_HAS_IPV4__
			else if (sa.sa.sa_family == AF_INET)
				pbuf = (char*)inet_ntop(AF_INET, &sa.sa4.sin_addr, pbuf, plen);
#endif
			DPRINTF("On try %d, sending query to %s, port %d\n",
				retries_left, pbuf, NAMESERVER_PORT);
			free(pbuf);
		}
#endif
		fd = socket(sa.sa.sa_family, SOCK_DGRAM, IPPROTO_UDP);
		if (fd < 0) /* paranoia */
			goto try_next_server;
		rc = connect(fd, &sa.sa, sizeof(sa));
		if (rc < 0) {
			/*if (errno == ENETUNREACH) { */
				/* routing error, presume not transient */
				goto try_next_server;
			/*} */
/*For example, what transient error this can be? Can't think of any */
			/* retry */
			/*continue; */
		}
		DPRINTF("Xmit packet len:%d id:%d qr:%d\n", packet_len, h.id, h.qr);
		/* no error check - if it fails, we time out on recv */
		send(fd, packet, packet_len, 0);

#ifdef USE_SELECT
		reply_timeout = __resolv_timeout;
 wait_again:
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = reply_timeout;
		tv.tv_usec = 0;
		if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0) {
			DPRINTF("Timeout\n");
			/* timed out, so retry send and receive
			 * to next nameserver */
			goto try_next_server;
		}
		reply_timeout--;
#else /* !USE_SELECT */
		reply_timeout = __resolv_timeout * 1000;
 wait_again:
		fds.fd = fd;
		fds.events = POLLIN;
		if (poll(&fds, 1, reply_timeout) <= 0) {
			DPRINTF("Timeout\n");
			/* timed out, so retry send and receive
			 * to next nameserver */
			goto try_next_server;
		}
		if (fds.revents & (POLLERR | POLLHUP | POLLNVAL)) {
			DPRINTF("Bad event\n");
			goto try_next_server;
		}
/*TODO: better timeout accounting?*/
		reply_timeout -= 1000;
#endif /* USE_SELECT */

/* vda: a bogus response seen in real world (caused SEGV in uclibc):
 * "ping www.google.com" sending AAAA query and getting
 * response with one answer... with answer part missing!
 * Fixed by thorough checks for not going past the packet's end.
 */
#ifdef DEBUG
		{
			static const char test_query[32] = "\0\2\1\0\0\1\0\0\0\0\0\0\3www\6google\3com\0\0\34\0\1";
			static const char test_respn[32] = "\0\2\201\200\0\1\0\1\0\0\0\0\3www\6google\3com\0\0\34\0\1";
			pos = memcmp(packet + 2, test_query + 2, 30);
		packet_len = recv(fd, packet, PACKETSZ, MSG_DONTWAIT);
			if (pos == 0) {
				packet_len = 32;
				memcpy(packet + 2, test_respn + 2, 30);
			}
		}
#else
		packet_len = recv(fd, packet, PACKETSZ, MSG_DONTWAIT);
#endif

		if (packet_len < HFIXEDSZ) {
			/* too short!
			 * If the peer did shutdown then retry later,
			 * try next peer on error.
			 * it's just a bogus packet from somewhere */
 bogus_packet:
			if (packet_len >= 0 && reply_timeout)
				goto wait_again;
			goto try_next_server;
		}
		__decode_header(packet, &h);
		DPRINTF("len:%d id:%d qr:%d\n", packet_len, h.id, h.qr);
		if (h.id != local_id || !h.qr) {
			/* unsolicited */
			goto bogus_packet;
		}

		DPRINTF("Got response (i think)!\n");
		DPRINTF("qrcount=%d,ancount=%d,nscount=%d,arcount=%d\n",
				h.qdcount, h.ancount, h.nscount, h.arcount);
		DPRINTF("opcode=%d,aa=%d,tc=%d,rd=%d,ra=%d,rcode=%d\n",
				h.opcode, h.aa, h.tc, h.rd, h.ra, h.rcode);

		/* bug 660 says we treat negative response as an error
		 * and retry, which is, eh, an error. :)
		 * We were incurring long delays because of this. */
		if (h.rcode == NXDOMAIN || h.rcode == SERVFAIL) {
			/* if possible, try next search domain */
			if (!ends_with_dot) {
				DPRINTF("variant:%d sdomains:%d\n", variant, sdomains);
				if (variant < sdomains - 1) {
					/* next search domain */
					variant++;
					continue;
				}
				/* no more search domains to try */
			}
			if (h.rcode != SERVFAIL) {
				/* dont loop, this is "no such host" situation */
				h_errno = HOST_NOT_FOUND;
				goto fail1;
			}
		}
		/* Insert other non-fatal errors here, which do not warrant
		 * switching to next nameserver */

		/* Strange error, assuming this nameserver is feeling bad */
		if (h.rcode != 0)
			goto try_next_server;

		/* Code below won't work correctly with h.ancount == 0, so... */
		if (h.ancount <= 0) {
			h_errno = NO_DATA; /* [is this correct code to check for?] */
			goto fail1;
		}
		pos = HFIXEDSZ;
		for (j = 0; j < h.qdcount; j++) {
			DPRINTF("Skipping question %d at %d\n", j, pos);
			i = __length_question(packet + pos, packet_len - pos);
			if (i < 0) {
				DPRINTF("Packet'question section "
					"is truncated, trying next server\n");
				goto try_next_server;
			}
			pos += i;
			DPRINTF("Length of question %d is %d\n", j, i);
		}
		DPRINTF("Decoding answer at pos %d\n", pos);

		first_answer = 1;
		a->dotted = NULL;
		for (j = 0; j < h.ancount; j++) {
			i = __decode_answer(packet, pos, packet_len, &ma);
			if (i < 0) {
				DPRINTF("failed decode %d\n", i);
				/* If the message was truncated but we have
				 * decoded some answers, pretend it's OK */
				if (j && h.tc)
					break;
				goto try_next_server;
			}
			pos += i;

			if (first_answer) {
				ma.buf = a->buf;
				ma.buflen = a->buflen;
				ma.add_count = a->add_count;
				free(a->dotted);
				memcpy(a, &ma, sizeof(ma));
				if (a->atype != T_SIG && (NULL == a->buf || (type != T_A && type != T_AAAA)))
					break;
				if (a->atype != type)
					continue;
				a->add_count = h.ancount - j - 1;
				if ((a->rdlength + sizeof(struct in_addr*)) * a->add_count > a->buflen)
					break;
				a->add_count = 0;
				first_answer = 0;
			} else {
				free(ma.dotted);
				if (ma.atype != type)
					continue;
				if (a->rdlength != ma.rdlength) {
					free(a->dotted);
					DPRINTF("Answer address len(%u) differs from original(%u)\n",
							ma.rdlength, a->rdlength);
					goto try_next_server;
				}
				memcpy(a->buf + (a->add_count * ma.rdlength), ma.rdata, ma.rdlength);
				++a->add_count;
			}
		}

		/* Success! */
		DPRINTF("Answer name = |%s|\n", a->dotted);
		DPRINTF("Answer type = |%d|\n", a->atype);
		if (fd != -1)
			close(fd);
		if (outpacket)
			*outpacket = packet;
		else
			free(packet);
		free(lookup);
		return packet_len;

 try_next_server:
		/* Try next nameserver */
		retries_left--;
		local_ns_num++;
		variant = -1;
	} while (retries_left > 0);

 fail:
	h_errno = NETDB_INTERNAL;
 fail1:
	if (fd != -1)
		close(fd);
	free(lookup);
	free(packet);
	return -1;
}