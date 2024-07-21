_gnutls_verify_certificate2 (gnutls_x509_crt_t cert,
			     const gnutls_x509_crt_t * trusted_cas,
			     int tcas_size, unsigned int flags,
			     unsigned int *output)
{
  gnutls_datum_t cert_signed_data = { NULL, 0 };
  gnutls_datum_t cert_signature = { NULL, 0 };
  gnutls_x509_crt_t issuer;
  int ret, issuer_version, result;

  if (output)
    *output = 0;

  if (tcas_size >= 1)
    issuer = find_issuer (cert, trusted_cas, tcas_size);
  else
    {
      gnutls_assert ();
      if (output)
	*output |= GNUTLS_CERT_SIGNER_NOT_FOUND | GNUTLS_CERT_INVALID;
      return 0;
    }

  /* issuer is not in trusted certificate
   * authorities.
   */
  if (issuer == NULL)
    {
      if (output)
	*output |= GNUTLS_CERT_SIGNER_NOT_FOUND | GNUTLS_CERT_INVALID;
      gnutls_assert ();
      return 0;
    }

  issuer_version = gnutls_x509_crt_get_version (issuer);
  if (issuer_version < 0)
    {
      gnutls_assert ();
      return issuer_version;
    }

  if (!(flags & GNUTLS_VERIFY_DISABLE_CA_SIGN) &&
      !((flags & GNUTLS_VERIFY_ALLOW_X509_V1_CA_CRT) && issuer_version == 1))
    {
      if (check_if_ca (cert, issuer, flags) == 0)
	{
	  gnutls_assert ();
	  if (output)
	    *output |= GNUTLS_CERT_SIGNER_NOT_CA | GNUTLS_CERT_INVALID;
	  return 0;
	}
    }

  result =
    _gnutls_x509_get_signed_data (cert->cert, "tbsCertificate",
				  &cert_signed_data);
  if (result < 0)
    {
      gnutls_assert ();
      goto cleanup;
    }

  result =
    _gnutls_x509_get_signature (cert->cert, "signature", &cert_signature);
  if (result < 0)
    {
      gnutls_assert ();
      goto cleanup;
    }

  ret =
    _gnutls_x509_verify_signature (&cert_signed_data, NULL, &cert_signature,
				   issuer);
  if (ret < 0)
    {
      gnutls_assert ();
    }
  else if (ret == 0)
    {
      gnutls_assert ();
      /* error. ignore it */
      if (output)
	*output |= GNUTLS_CERT_INVALID;
      ret = 0;
    }

  /* If the certificate is not self signed check if the algorithms
   * used are secure. If the certificate is self signed it doesn't
   * really matter.
   */
  if (is_issuer (cert, cert) == 0)
    {
      int sigalg;

      sigalg = gnutls_x509_crt_get_signature_algorithm (cert);

      if (((sigalg == GNUTLS_SIGN_RSA_MD2) &&
	   !(flags & GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD2)) ||
	  ((sigalg == GNUTLS_SIGN_RSA_MD5) &&
	   !(flags & GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD5)))
	{
	  if (output)
	    *output |= GNUTLS_CERT_INSECURE_ALGORITHM | GNUTLS_CERT_INVALID;
	  ret = 0;
	}
    }

  result = ret;

cleanup:
  _gnutls_free_datum (&cert_signed_data);
  _gnutls_free_datum (&cert_signature);

  return result;
}