gnutls_pcert_deinit (gnutls_pcert_st *pcert)
{
  gnutls_pubkey_deinit(pcert->pubkey);
  pcert->pubkey = NULL;
  gnutls_free(pcert->cert.data);
  pcert->cert.data = NULL;
}