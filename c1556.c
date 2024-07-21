int dane_query_tlsa(dane_state_t s, dane_query_t *r, const char* host, const char* proto, unsigned int port)
{
	char ns[1024];
	int ret;
	unsigned int i;

	*r = calloc(1, sizeof(struct dane_query_st));
	if (*r == NULL)
		return gnutls_assert_val(DANE_E_MEMORY_ERROR);

	snprintf(ns, sizeof(ns), "_%u._%s.%s", port, proto, host);

	/* query for webserver */
	ret = ub_resolve(s->ctx, ns, 52, 1, &(*r)->result);
	if(ret != 0) {
		return gnutls_assert_val(DANE_E_RESOLVING_ERROR);
	}

/* show first result */
	if(!(*r)->result->havedata) {
		return gnutls_assert_val(DANE_E_NO_DANE_DATA);
	}

	i = 0;
	do {

		if ((*r)->result->len[i] > 3)
			ret = DANE_E_SUCCESS;
		else {
			return gnutls_assert_val(DANE_E_RECEIVED_CORRUPT_DATA);
		}
	
		(*r)->usage[i] = (*r)->result->data[i][0];
		(*r)->type[i] = (*r)->result->data[i][1];
		(*r)->match[i] = (*r)->result->data[i][2];
		(*r)->data[i].data = (void*)&(*r)->result->data[i][3];
		(*r)->data[i].size = (*r)->result->len[i] - 3;
		i++;
	} while((*r)->result->data[i] != NULL);
	
	(*r)->data_entries = i;

	if (!(s->flags & DANE_F_INSECURE) && !(*r)->result->secure) {
		if ((*r)->result->bogus)
			ret = gnutls_assert_val(DANE_E_INVALID_DNSSEC_SIG);
		else
			ret = gnutls_assert_val(DANE_E_NO_DNSSEC_SIG);
	}

	/* show security status */
	if ((*r)->result->secure) {
		(*r)->status = DANE_QUERY_DNSSEC_VERIFIED;
	} else if ((*r)->result->bogus) {
	        gnutls_assert();
		(*r)->status = DANE_QUERY_BOGUS;
	} else {
	        gnutls_assert();
	        (*r)->status = DANE_QUERY_NO_DNSSEC;
        }

	return ret;
}