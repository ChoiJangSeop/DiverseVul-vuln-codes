int pn_ssl_domain_set_peer_authentication(pn_ssl_domain_t *domain,
                                          const pn_ssl_verify_mode_t mode,
                                          const char *trusted_CAs)
{
  if (!domain) return -1;

  switch (mode) {
   case PN_SSL_VERIFY_PEER:
   case PN_SSL_VERIFY_PEER_NAME:

#ifdef SSL_SECOP_PEER
    SSL_CTX_set_security_level(domain->ctx, domain->default_seclevel);
#endif

    if (!domain->has_ca_db) {
      pn_transport_logf(NULL, "Error: cannot verify peer without a trusted CA configured, use pn_ssl_domain_set_trusted_ca_db()");
      return -1;
    }

    if (domain->mode == PN_SSL_MODE_SERVER) {
      // openssl requires that server connections supply a list of trusted CAs which is
      // sent to the client
      if (!trusted_CAs) {
        pn_transport_logf(NULL, "Error: a list of trusted CAs must be provided.");
        return -1;
      }
      if (!domain->has_certificate) {
        pn_transport_logf(NULL, "Error: Server cannot verify peer without configuring a certificate, use pn_ssl_domain_set_credentials()");
      }

      if (domain->trusted_CAs) free(domain->trusted_CAs);
      domain->trusted_CAs = pn_strdup( trusted_CAs );
      STACK_OF(X509_NAME) *cert_names;
      cert_names = SSL_load_client_CA_file( domain->trusted_CAs );
      if (cert_names != NULL)
        SSL_CTX_set_client_CA_list(domain->ctx, cert_names);
      else {
        pn_transport_logf(NULL, "Error: Unable to process file of trusted CAs: %s", trusted_CAs);
        return -1;
      }
    }

    SSL_CTX_set_verify( domain->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                        verify_callback);
#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
    SSL_CTX_set_verify_depth(domain->ctx, 1);
#endif
    break;

   case PN_SSL_ANONYMOUS_PEER:   // hippie free love mode... :)
#ifdef SSL_SECOP_PEER
    // Must use lowest OpenSSL security level to enable anonymous ciphers.
    SSL_CTX_set_security_level(domain->ctx, 0);
#endif
    SSL_CTX_set_verify( domain->ctx, SSL_VERIFY_NONE, NULL );
    break;

   default:
    pn_transport_logf(NULL, "Invalid peer authentication mode given." );
    return -1;
  }

  domain->verify_mode = mode;
  return 0;
}