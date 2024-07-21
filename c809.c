int gnutls_pcert_import_x509 (gnutls_pcert_st* pcert,
	gnutls_x509_crt_t crt, unsigned int flags)
{
int ret;
size_t sz;

  memset(pcert, 0, sizeof(*pcert));

  pcert->type = GNUTLS_CRT_X509;
  pcert->cert.data = NULL;

  sz = 0;
  ret = gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_DER, NULL, &sz);
  if (ret < 0 && ret != GNUTLS_E_SHORT_MEMORY_BUFFER)
    {
      ret = gnutls_assert_val(ret);
      goto cleanup;
    }

  pcert->cert.data = gnutls_malloc(sz);
  if (pcert->cert.data == NULL)
    {
      ret = gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);
      goto cleanup;
    }

  ret = gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_DER, pcert->cert.data, &sz);
  if (ret < 0)
    {
      ret = gnutls_assert_val(ret);
      goto cleanup;
    }
  pcert->cert.size = sz;

  ret = gnutls_pubkey_init(&pcert->pubkey);
  if (ret < 0)
    {
      ret = gnutls_assert_val(ret);
      goto cleanup;
    }

  ret = gnutls_pubkey_import_x509(pcert->pubkey, crt, 0);
  if (ret < 0)
    {
      gnutls_pubkey_deinit(pcert->pubkey);
      ret = gnutls_assert_val(ret);
      goto cleanup;
    }

  return 0;

cleanup:
  gnutls_free(pcert->cert.data);

  return ret;
}