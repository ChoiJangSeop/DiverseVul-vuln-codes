static int parse_response(unsigned char *buf, int buflen,
			char *question, int qlen,
			uint16_t *type, uint16_t *class, int *ttl,
			unsigned char *response, unsigned int *response_len,
			uint16_t *answers)
{
	struct domain_hdr *hdr = (void *) buf;
	struct domain_question *q;
	unsigned char *ptr;
	uint16_t qdcount = ntohs(hdr->qdcount);
	uint16_t ancount = ntohs(hdr->ancount);
	int err, i;
	uint16_t qtype, qclass;
	unsigned char *next = NULL;
	unsigned int maxlen = *response_len;
	GSList *aliases = NULL, *list;
	char name[NS_MAXDNAME + 1];

	if (buflen < 12)
		return -EINVAL;

	debug("qr %d qdcount %d", hdr->qr, qdcount);

	/* We currently only cache responses where question count is 1 */
	if (hdr->qr != 1 || qdcount != 1)
		return -EINVAL;

	ptr = buf + sizeof(struct domain_hdr);

	strncpy(question, (char *) ptr, qlen);
	qlen = strlen(question);
	ptr += qlen + 1; /* skip \0 */

	q = (void *) ptr;
	qtype = ntohs(q->type);

	/* We cache only A and AAAA records */
	if (qtype != 1 && qtype != 28)
		return -ENOMSG;

	qclass = ntohs(q->class);

	ptr += 2 + 2; /* ptr points now to answers */

	err = -ENOMSG;
	*response_len = 0;
	*answers = 0;

	memset(name, 0, sizeof(name));

	/*
	 * We have a bunch of answers (like A, AAAA, CNAME etc) to
	 * A or AAAA question. We traverse the answers and parse the
	 * resource records. Only A and AAAA records are cached, all
	 * the other records in answers are skipped.
	 */
	for (i = 0; i < ancount; i++) {
		/*
		 * Get one address at a time to this buffer.
		 * The max size of the answer is
		 *   2 (pointer) + 2 (type) + 2 (class) +
		 *   4 (ttl) + 2 (rdlen) + addr (16 or 4) = 28
		 * for A or AAAA record.
		 * For CNAME the size can be bigger.
		 */
		unsigned char rsp[NS_MAXCDNAME];
		unsigned int rsp_len = sizeof(rsp) - 1;
		int ret, rdlen;

		memset(rsp, 0, sizeof(rsp));

		ret = parse_rr(buf, ptr, buf + buflen, rsp, &rsp_len,
			type, class, ttl, &rdlen, &next, name);
		if (ret != 0) {
			err = ret;
			goto out;
		}

		/*
		 * Now rsp contains compressed or uncompressed resource
		 * record. Next we check if this record answers the question.
		 * The name var contains the uncompressed label.
		 * One tricky bit is the CNAME records as they alias
		 * the name we might be interested in.
		 */

		/*
		 * Go to next answer if the class is not the one we are
		 * looking for.
		 */
		if (*class != qclass) {
			ptr = next;
			next = NULL;
			continue;
		}

		/*
		 * Try to resolve aliases also, type is CNAME(5).
		 * This is important as otherwise the aliased names would not
		 * be cached at all as the cache would not contain the aliased
		 * question.
		 *
		 * If any CNAME is found in DNS packet, then we cache the alias
		 * IP address instead of the question (as the server
		 * said that question has only an alias).
		 * This means in practice that if e.g., ipv6.google.com is
		 * queried, DNS server returns CNAME of that name which is
		 * ipv6.l.google.com. We then cache the address of the CNAME
		 * but return the question name to client. So the alias
		 * status of the name is not saved in cache and thus not
		 * returned to the client. We do not return DNS packets from
		 * cache to client saying that ipv6.google.com is an alias to
		 * ipv6.l.google.com but we return instead a DNS packet that
		 * says ipv6.google.com has address xxx which is in fact the
		 * address of ipv6.l.google.com. For caching purposes this
		 * should not cause any issues.
		 */
		if (*type == 5 && strncmp(question, name, qlen) == 0) {
			/*
			 * So now the alias answered the question. This is
			 * not very useful from caching point of view as
			 * the following A or AAAA records will not match the
			 * question. We need to find the real A/AAAA record
			 * of the alias and cache that.
			 */
			unsigned char *end = NULL;
			int name_len = 0, output_len = 0;

			memset(rsp, 0, sizeof(rsp));
			rsp_len = sizeof(rsp) - 1;

			/*
			 * Alias is in rdata part of the message,
			 * and next-rdlen points to it. So we need to get
			 * the real name of the alias.
			 */
			ret = get_name(0, buf, next - rdlen, buf + buflen,
					rsp, rsp_len, &output_len, &end,
					name, &name_len);
			if (ret != 0) {
				/* just ignore the error at this point */
				ptr = next;
				next = NULL;
				continue;
			}

			/*
			 * We should now have the alias of the entry we might
			 * want to cache. Just remember it for a while.
			 * We check the alias list when we have parsed the
			 * A or AAAA record.
			 */
			aliases = g_slist_prepend(aliases, g_strdup(name));

			ptr = next;
			next = NULL;
			continue;
		}

		if (*type == qtype) {
			/*
			 * We found correct type (A or AAAA)
			 */
			if (check_alias(aliases, name) ||
				(!aliases && strncmp(question, name,
							qlen) == 0)) {
				/*
				 * We found an alias or the name of the rr
				 * matches the question. If so, we append
				 * the compressed label to the cache.
				 * The end result is a response buffer that
				 * will contain one or more cached and
				 * compressed resource records.
				 */
				if (*response_len + rsp_len > maxlen) {
					err = -ENOBUFS;
					goto out;
				}
				memcpy(response + *response_len, rsp, rsp_len);
				*response_len += rsp_len;
				(*answers)++;
				err = 0;
			}
		}

		ptr = next;
		next = NULL;
	}

out:
	for (list = aliases; list; list = list->next)
		g_free(list->data);
	g_slist_free(aliases);

	return err;
}