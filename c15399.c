res_nmkquery(res_state statp,
	     int op,			/* opcode of query */
	     const char *dname,		/* domain name */
	     int class, int type,	/* class and type of query */
	     const u_char *data,	/* resource record data */
	     int datalen,		/* length of data */
	     const u_char *newrr_in,	/* new rr for modify or append */
	     u_char *buf,		/* buffer to put query */
	     int buflen)		/* size of buffer */
{
	HEADER *hp;
	u_char *cp;
	int n;
	u_char *dnptrs[20], **dpp, **lastdnptr;

#ifdef DEBUG
	if (statp->options & RES_DEBUG)
		printf(";; res_nmkquery(%s, %s, %s, %s)\n",
		       _res_opcodes[op], dname, p_class(class), p_type(type));
#endif
	/*
	 * Initialize header fields.
	 */
	if ((buf == NULL) || (buflen < HFIXEDSZ))
		return (-1);
	memset(buf, 0, HFIXEDSZ);
	hp = (HEADER *) buf;
	/* We randomize the IDs every time.  The old code just
	   incremented by one after the initial randomization which
	   still predictable if the application does multiple
	   requests.  */
	int randombits;
	do
	  {
#ifdef RANDOM_BITS
	    RANDOM_BITS (randombits);
#else
	    struct timeval tv;
	    __gettimeofday (&tv, NULL);
	    randombits = (tv.tv_sec << 8) ^ tv.tv_usec;
#endif
	  }
	while ((randombits & 0xffff) == 0);
	statp->id = (statp->id + randombits) & 0xffff;
	hp->id = statp->id;
	hp->opcode = op;
	hp->rd = (statp->options & RES_RECURSE) != 0;
	hp->rcode = NOERROR;
	cp = buf + HFIXEDSZ;
	buflen -= HFIXEDSZ;
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof dnptrs / sizeof dnptrs[0];
	/*
	 * perform opcode specific processing
	 */
	switch (op) {
	case NS_NOTIFY_OP:
		if ((buflen -= QFIXEDSZ + (data == NULL ? 0 : RRFIXEDSZ)) < 0)
			return (-1);
		goto compose;

	case QUERY:
		if ((buflen -= QFIXEDSZ) < 0)
			return (-1);
	compose:
		n = ns_name_compress(dname, cp, buflen,
				     (const u_char **) dnptrs,
				     (const u_char **) lastdnptr);
		if (n < 0)
			return (-1);
		cp += n;
		buflen -= n;
		NS_PUT16 (type, cp);
		NS_PUT16 (class, cp);
		hp->qdcount = htons(1);
		if (op == QUERY || data == NULL)
			break;
		/*
		 * Make an additional record for completion domain.
		 */
		n = ns_name_compress((char *)data, cp, buflen,
				     (const u_char **) dnptrs,
				     (const u_char **) lastdnptr);
		if (__glibc_unlikely (n < 0))
			return (-1);
		cp += n;
		buflen -= n;
		NS_PUT16 (T_NULL, cp);
		NS_PUT16 (class, cp);
		NS_PUT32 (0, cp);
		NS_PUT16 (0, cp);
		hp->arcount = htons(1);
		break;

	case IQUERY:
		/*
		 * Initialize answer section
		 */
		if (__glibc_unlikely (buflen < 1 + RRFIXEDSZ + datalen))
			return (-1);
		*cp++ = '\0';	/* no domain name */
		NS_PUT16 (type, cp);
		NS_PUT16 (class, cp);
		NS_PUT32 (0, cp);
		NS_PUT16 (datalen, cp);
		if (datalen) {
			memcpy(cp, data, datalen);
			cp += datalen;
		}
		hp->ancount = htons(1);
		break;

	default:
		return (-1);
	}
	return (cp - buf);
}