int dane_verify_crt (dane_state_t s,
	const gnutls_datum_t *chain, unsigned chain_size,
	gnutls_certificate_type_t chain_type,
	const char * hostname, const char* proto, unsigned int port,
	unsigned int sflags, unsigned int vflags,
	unsigned int *verify)
{
dane_state_t _s = NULL;
dane_query_t r = NULL;
int ret;
unsigned checked = 0;
unsigned int usage, type, match, idx;
gnutls_datum_t data;
	
	if (chain_type != GNUTLS_CRT_X509)
		return gnutls_assert_val(DANE_E_INVALID_REQUEST);
	
	*verify = 0;
	
	if (s == NULL) {
		ret = dane_state_init(&_s, sflags);
		if (ret < 0) {
		        gnutls_assert();
			return ret;
		}
	} else
		_s = s;
	
	ret = dane_query_tlsa(_s, &r, hostname, proto, port);
	if (ret < 0) {
	        gnutls_assert();
		goto cleanup;
	}

	idx = 0;
	do {
		ret = dane_query_data(r, idx++, &usage, &type, &match, &data);
		if (ret == DANE_E_REQUESTED_DATA_NOT_AVAILABLE)
			break;

		if (ret < 0) {
			gnutls_assert();
			goto cleanup;
		}
		
		if (!(vflags & DANE_VFLAG_ONLY_CHECK_EE_USAGE) && (usage == DANE_CERT_USAGE_LOCAL_CA || usage == DANE_CERT_USAGE_CA)) {
			ret = verify_ca(chain, chain_size, chain_type, type, match, &data, verify);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
                        }
                        checked = 1;
		} else if (!(vflags & DANE_VFLAG_ONLY_CHECK_CA_USAGE) && (usage == DANE_CERT_USAGE_LOCAL_EE || usage == DANE_CERT_USAGE_EE)) {
			ret = verify_ee(&chain[0], chain_type, type, match, &data, verify);
			if (ret < 0) {
				gnutls_assert();
				goto cleanup;
                        }
                        checked = 1;
		}
	} while(1);

	if ((vflags & DANE_VFLAG_FAIL_IF_NOT_CHECKED) && checked == 0)
		ret = gnutls_assert_val(DANE_E_REQUESTED_DATA_NOT_AVAILABLE);
	else
		ret = 0;

cleanup:
	if (s == NULL) dane_state_deinit(_s);
	if (r != NULL) dane_query_deinit(r);
	return ret;
}