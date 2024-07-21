int gethostbyaddr_r(const void *addr, socklen_t addrlen,
		int type,
		struct hostent *result_buf,
		char *buf, size_t buflen,
		struct hostent **result,
		int *h_errnop)

{
	struct in_addr *in;
	struct in_addr **addr_list;
	char **alias;
	unsigned char *packet;
	struct resolv_answer a;
	int i;
	int packet_len;
	int nest = 0;

	*result = NULL;
	if (!addr)
		return EINVAL;

	switch (type) {
#ifdef __UCLIBC_HAS_IPV4__
		case AF_INET:
			if (addrlen != sizeof(struct in_addr))
				return EINVAL;
			break;
#endif
#ifdef __UCLIBC_HAS_IPV6__
		case AF_INET6:
			if (addrlen != sizeof(struct in6_addr))
				return EINVAL;
			break;
#endif
		default:
			return EINVAL;
	}

	/* do /etc/hosts first */
	i = __get_hosts_byaddr_r(addr, addrlen, type, result_buf,
				buf, buflen, result, h_errnop);
	if (i == 0)
		return i;
	switch (*h_errnop) {
		case HOST_NOT_FOUND:
		case NO_ADDRESS:
			break;
		default:
			return i;
	}

	*h_errnop = NETDB_INTERNAL;

	/* make sure pointer is aligned */
	i = ALIGN_BUFFER_OFFSET(buf);
	buf += i;
	buflen -= i;
	/* Layout in buf:
	 * char *alias[ALIAS_DIM];
	 * struct in[6]_addr* addr_list[2];
	 * struct in[6]_addr in;
	 * char scratch_buffer[256+];
	 */
#define in6 ((struct in6_addr *)in)
	alias = (char **)buf;
	addr_list = (struct in_addr**)buf;
	buf += sizeof(*addr_list) * 2;
	buflen -= sizeof(*addr_list) * 2;
	in = (struct in_addr*)buf;
#ifndef __UCLIBC_HAS_IPV6__
	buf += sizeof(*in);
	buflen -= sizeof(*in);
	if (addrlen > sizeof(*in))
		return ERANGE;
#else
	buf += sizeof(*in6);
	buflen -= sizeof(*in6);
	if (addrlen > sizeof(*in6))
		return ERANGE;
#endif
	if ((ssize_t)buflen < 256)
		return ERANGE;
	alias[0] = buf;
	alias[1] = NULL;
	addr_list[0] = in;
	addr_list[1] = NULL;
	memcpy(in, addr, addrlen);

	if (0) /* nothing */;
#ifdef __UCLIBC_HAS_IPV4__
	else IF_HAS_BOTH(if (type == AF_INET)) {
		unsigned char *tp = (unsigned char *)addr;
		sprintf(buf, "%u.%u.%u.%u.in-addr.arpa",
				tp[3], tp[2], tp[1], tp[0]);
	}
#endif
#ifdef __UCLIBC_HAS_IPV6__
	else {
		char *dst = buf;
		unsigned char *tp = (unsigned char *)addr + addrlen - 1;
		do {
			dst += sprintf(dst, "%x.%x.", tp[0] & 0xf, tp[0] >> 4);
			tp--;
		} while (tp >= (unsigned char *)addr);
		strcpy(dst, "ip6.arpa");
	}
#endif

	memset(&a, '\0', sizeof(a));
	for (;;) {
/* Hmm why we memset(a) to zeros only once? */
		packet_len = __dns_lookup(buf, T_PTR, &packet, &a);
		if (packet_len < 0) {
			*h_errnop = HOST_NOT_FOUND;
			return TRY_AGAIN;
		}

		strncpy(buf, a.dotted, buflen);
		free(a.dotted);
		if (a.atype != T_CNAME)
			break;

		DPRINTF("Got a CNAME in gethostbyaddr()\n");
		if (++nest > MAX_RECURSE) {
			*h_errnop = NO_RECOVERY;
			return -1;
		}
		/* Decode CNAME into buf, feed it to __dns_lookup() again */
		i = __decode_dotted(packet, a.rdoffset, packet_len, buf, buflen);
		free(packet);
		if (i < 0) {
			*h_errnop = NO_RECOVERY;
			return -1;
		}
	}

	if (a.atype == T_PTR) {	/* ADDRESS */
		i = __decode_dotted(packet, a.rdoffset, packet_len, buf, buflen);
		free(packet);
		result_buf->h_name = buf;
		result_buf->h_addrtype = type;
		result_buf->h_length = addrlen;
		result_buf->h_addr_list = (char **) addr_list;
		result_buf->h_aliases = alias;
		*result = result_buf;
		*h_errnop = NETDB_SUCCESS;
		return NETDB_SUCCESS;
	}

	free(packet);
	*h_errnop = NO_ADDRESS;
	return TRY_AGAIN;
#undef in6
}