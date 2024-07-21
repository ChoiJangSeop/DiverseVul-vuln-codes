int dane_verify_session_crt (
        dane_state_t s,
	gnutls_session_t session,
	const char * hostname, const char* proto, unsigned int port,
	unsigned int sflags, unsigned int vflags,
	unsigned int *verify)
{
const gnutls_datum_t *cert_list;
unsigned int cert_list_size = 0;
unsigned int type;

	cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
	if (cert_list_size == 0) {
		return gnutls_assert_val(DANE_E_NO_CERT);
	}
	
	type = gnutls_certificate_type_get(session);
	
	return dane_verify_crt(s, cert_list, cert_list_size, type, hostname, proto, port, sflags, vflags, verify);
}