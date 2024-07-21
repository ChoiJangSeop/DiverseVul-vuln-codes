_gnutls_proc_x509_server_certificate (gnutls_session_t session,
                                      uint8_t * data, size_t data_size)
{
  int size, len, ret;
  uint8_t *p = data;
  cert_auth_info_t info;
  gnutls_certificate_credentials_t cred;
  ssize_t dsize = data_size;
  int i;
  gnutls_pcert_st *peer_certificate_list;
  size_t peer_certificate_list_size = 0, j, x;
  gnutls_datum_t tmp;

  cred = (gnutls_certificate_credentials_t)
    _gnutls_get_cred (session->key, GNUTLS_CRD_CERTIFICATE, NULL);
  if (cred == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
    }


  if ((ret =
       _gnutls_auth_info_set (session, GNUTLS_CRD_CERTIFICATE,
                              sizeof (cert_auth_info_st), 1)) < 0)
    {
      gnutls_assert ();
      return ret;
    }

  info = _gnutls_get_auth_info (session);

  if (data == NULL || data_size == 0)
    {
      gnutls_assert ();
      /* no certificate was sent */
      return GNUTLS_E_NO_CERTIFICATE_FOUND;
    }

  DECR_LEN (dsize, 3);
  size = _gnutls_read_uint24 (p);
  p += 3;

  /* some implementations send 0B 00 00 06 00 00 03 00 00 00
   * instead of just 0B 00 00 03 00 00 00 as an empty certificate message.
   */
  if (size == 0 || size == 3)
    {
      gnutls_assert ();
      /* no certificate was sent */
      return GNUTLS_E_NO_CERTIFICATE_FOUND;
    }

  i = dsize;
  while (i > 0)
    {
      DECR_LEN (dsize, 3);
      len = _gnutls_read_uint24 (p);
      p += 3;
      DECR_LEN (dsize, len);
      peer_certificate_list_size++;
      p += len;
      i -= len + 3;
    }

  if (peer_certificate_list_size == 0)
    {
      gnutls_assert ();
      return GNUTLS_E_NO_CERTIFICATE_FOUND;
    }

  /* Ok we now allocate the memory to hold the
   * certificate list 
   */

  peer_certificate_list =
    gnutls_calloc (1,
                   sizeof (gnutls_pcert_st) * (peer_certificate_list_size));
  if (peer_certificate_list == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  p = data + 3;

  /* Now we start parsing the list (again).
   * We don't use DECR_LEN since the list has
   * been parsed before.
   */

  for (j = 0; j < peer_certificate_list_size; j++)
    {
      len = _gnutls_read_uint24 (p);
      p += 3;

      tmp.size = len;
      tmp.data = p;

      ret =
        gnutls_pcert_import_x509_raw (&peer_certificate_list
                                      [j], &tmp, GNUTLS_X509_FMT_DER, 0);
      if (ret < 0)
        {
          gnutls_assert ();
          goto cleanup;
        }

      p += len;
    }


  if ((ret =
       _gnutls_copy_certificate_auth_info (info,
                                           peer_certificate_list,
                                           peer_certificate_list_size, 0,
                                           NULL)) < 0)
    {
      gnutls_assert ();
      goto cleanup;
    }

  if ((ret =
       _gnutls_check_key_usage (&peer_certificate_list[0],
                                gnutls_kx_get (session))) < 0)
    {
      gnutls_assert ();
      goto cleanup;
    }

  ret = 0;

cleanup:
  CLEAR_CERTS;
  gnutls_free (peer_certificate_list);
  return ret;

}