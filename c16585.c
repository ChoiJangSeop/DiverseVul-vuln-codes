static X509_STORE *init_revocation_store(EAP_TLS_CONF *conf)
{
	X509_STORE *store = NULL;

	store = X509_STORE_new();

	/* Load the CAs we trust */
        if (conf->ca_file || conf->ca_path)
		if(!X509_STORE_load_locations(store, conf->ca_file, conf->ca_path)) {
			radlog(L_ERR, "rlm_eap: X509_STORE error %s", ERR_error_string(ERR_get_error(), NULL));
			radlog(L_ERR, "rlm_eap_tls: Error reading Trusted root CA list %s",conf->ca_file );
			return NULL;
		}

#ifdef X509_V_FLAG_CRL_CHECK
	if (conf->check_crl)
		X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK);
#endif
	return store;
}