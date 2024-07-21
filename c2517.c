gnutls_x509_crt_verify (gnutls_x509_crt_t cert,
			const gnutls_x509_crt_t * CA_list,
			int CA_list_length, unsigned int flags,
			unsigned int *verify)
{
  int ret;
  /* Verify certificate 
   */
  ret =
    _gnutls_verify_certificate2 (cert, CA_list, CA_list_length, flags,
				 verify);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  return 0;
}