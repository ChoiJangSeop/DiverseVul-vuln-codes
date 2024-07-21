int gnutls_pcert_import_openpgp (gnutls_pcert_st* pcert,
	gnutls_openpgp_crt_t crt, unsigned int flags)
{
int ret;
size_t sz;

  memset(pcert, 0, sizeof(*pcert));

  pcert->type = GNUTLS_CRT_OPENPGP;
  pcert->cert.data = NULL;

  sz = 0;
  ret = gnutls_openpgp_crt_export(crt, GNUTLS_OPENPGP_FMT_RAW, NULL, &sz);
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

  ret = gnutls_openpgp_crt_export(crt, GNUTLS_X509_FMT_DER, pcert->cert.data, &sz);
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

  ret = gnutls_pubkey_import_openpgp(pcert->pubkey, crt, 0);
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