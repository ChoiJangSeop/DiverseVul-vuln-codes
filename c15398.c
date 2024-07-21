__libc_res_nquery(res_state statp,
		  const char *name,	/* domain name */
		  int class, int type,	/* class and type of query */
		  u_char *answer,	/* buffer to put answer */
		  int anslen,		/* size of answer buffer */
		  u_char **answerp,	/* if buffer needs to be enlarged */
		  u_char **answerp2,
		  int *nanswerp2,
		  int *resplen2,
		  int *answerp2_malloced)
{
	HEADER *hp = (HEADER *) answer;
	HEADER *hp2;
	int n, use_malloc = 0;
	u_int oflags = statp->_flags;

	size_t bufsize = (type == T_UNSPEC ? 2 : 1) * QUERYSIZE;
	u_char *buf = alloca (bufsize);
	u_char *query1 = buf;
	int nquery1 = -1;
	u_char *query2 = NULL;
	int nquery2 = 0;

 again:
	hp->rcode = NOERROR;	/* default */

#ifdef DEBUG
	if (statp->options & RES_DEBUG)
		printf(";; res_query(%s, %d, %d)\n", name, class, type);
#endif

	if (type == T_UNSPEC)
	  {
	    n = res_nmkquery(statp, QUERY, name, class, T_A, NULL, 0, NULL,
			     query1, bufsize);
	    if (n > 0)
	      {
		if ((oflags & RES_F_EDNS0ERR) == 0
		    && (statp->options & (RES_USE_EDNS0|RES_USE_DNSSEC)) != 0)
		  {
		    n = __res_nopt(statp, n, query1, bufsize, anslen / 2);
		    if (n < 0)
		      goto unspec_nomem;
		  }

		nquery1 = n;
		/* Align the buffer.  */
		int npad = ((nquery1 + __alignof__ (HEADER) - 1)
			    & ~(__alignof__ (HEADER) - 1)) - nquery1;
		if (n > bufsize - npad)
		  {
		    n = -1;
		    goto unspec_nomem;
		  }
		int nused = n + npad;
		query2 = buf + nused;
		n = res_nmkquery(statp, QUERY, name, class, T_AAAA, NULL, 0,
				 NULL, query2, bufsize - nused);
		if (n > 0
		    && (oflags & RES_F_EDNS0ERR) == 0
		    && (statp->options & (RES_USE_EDNS0|RES_USE_DNSSEC)) != 0)
		  n = __res_nopt(statp, n, query2, bufsize - nused - n,
				 anslen / 2);
		nquery2 = n;
	      }

	  unspec_nomem:;
	  }
	else
	  {
	    n = res_nmkquery(statp, QUERY, name, class, type, NULL, 0, NULL,
			     query1, bufsize);

	    if (n > 0
		&& (oflags & RES_F_EDNS0ERR) == 0
		&& (statp->options & (RES_USE_EDNS0|RES_USE_DNSSEC)) != 0)
	      n = __res_nopt(statp, n, query1, bufsize, anslen);

	    nquery1 = n;
	  }

	if (__builtin_expect (n <= 0, 0) && !use_malloc) {
		/* Retry just in case res_nmkquery failed because of too
		   short buffer.  Shouldn't happen.  */
		bufsize = (type == T_UNSPEC ? 2 : 1) * MAXPACKET;
		buf = malloc (bufsize);
		if (buf != NULL) {
			query1 = buf;
			use_malloc = 1;
			goto again;
		}
	}
	if (__glibc_unlikely (n <= 0))       {
		/* If the query choked with EDNS0, retry without EDNS0.  */
		if ((statp->options & (RES_USE_EDNS0|RES_USE_DNSSEC)) != 0
		    && ((oflags ^ statp->_flags) & RES_F_EDNS0ERR) != 0) {
			statp->_flags |= RES_F_EDNS0ERR;
#ifdef DEBUG
			if (statp->options & RES_DEBUG)
				printf(";; res_nquery: retry without EDNS0\n");
#endif
			goto again;
		}
#ifdef DEBUG
		if (statp->options & RES_DEBUG)
			printf(";; res_query: mkquery failed\n");
#endif
		RES_SET_H_ERRNO(statp, NO_RECOVERY);
		if (use_malloc)
			free (buf);
		return (n);
	}
	assert (answerp == NULL || (void *) *answerp == (void *) answer);
	n = __libc_res_nsend(statp, query1, nquery1, query2, nquery2, answer,
			     anslen, answerp, answerp2, nanswerp2, resplen2,
			     answerp2_malloced);
	if (use_malloc)
		free (buf);
	if (n < 0) {
#ifdef DEBUG
		if (statp->options & RES_DEBUG)
			printf(";; res_query: send error\n");
#endif
		RES_SET_H_ERRNO(statp, TRY_AGAIN);
		return (n);
	}

	if (answerp != NULL)
	  /* __libc_res_nsend might have reallocated the buffer.  */
	  hp = (HEADER *) *answerp;

	/* We simplify the following tests by assigning HP to HP2 or
	   vice versa.  It is easy to verify that this is the same as
	   ignoring all tests of HP or HP2.  */
	if (answerp2 == NULL || *resplen2 < (int) sizeof (HEADER))
	  {
	    hp2 = hp;
	  }
	else
	  {
	    hp2 = (HEADER *) *answerp2;
	    if (n < (int) sizeof (HEADER))
	      {
	        hp = hp2;
	      }
	  }

	/* Make sure both hp and hp2 are defined */
	assert((hp != NULL) && (hp2 != NULL));

	if ((hp->rcode != NOERROR || ntohs(hp->ancount) == 0)
	    && (hp2->rcode != NOERROR || ntohs(hp2->ancount) == 0)) {
#ifdef DEBUG
		if (statp->options & RES_DEBUG) {
			printf(";; rcode = %d, ancount=%d\n", hp->rcode,
			    ntohs(hp->ancount));
			if (hp != hp2)
			  printf(";; rcode2 = %d, ancount2=%d\n", hp2->rcode,
				 ntohs(hp2->ancount));
		}
#endif
		switch (hp->rcode == NOERROR ? hp2->rcode : hp->rcode) {
		case NXDOMAIN:
			if ((hp->rcode == NOERROR && ntohs (hp->ancount) != 0)
			    || (hp2->rcode == NOERROR
				&& ntohs (hp2->ancount) != 0))
				goto success;
			RES_SET_H_ERRNO(statp, HOST_NOT_FOUND);
			break;
		case SERVFAIL:
			RES_SET_H_ERRNO(statp, TRY_AGAIN);
			break;
		case NOERROR:
			if (ntohs (hp->ancount) != 0
			    || ntohs (hp2->ancount) != 0)
				goto success;
			RES_SET_H_ERRNO(statp, NO_DATA);
			break;
		case FORMERR:
		case NOTIMP:
			/* Servers must not reply to AAAA queries with
			   NOTIMP etc but some of them do.  */
			if ((hp->rcode == NOERROR && ntohs (hp->ancount) != 0)
			    || (hp2->rcode == NOERROR
				&& ntohs (hp2->ancount) != 0))
				goto success;
			/* FALLTHROUGH */
		case REFUSED:
		default:
			RES_SET_H_ERRNO(statp, NO_RECOVERY);
			break;
		}
		return (-1);
	}
 success:
	return (n);
}