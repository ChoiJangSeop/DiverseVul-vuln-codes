int gnutls_x509_crt_get_serial(gnutls_x509_crt_t cert, void *result,
			       size_t * result_size)
{
    int ret;

    if (cert == NULL) {
	gnutls_assert();
	return GNUTLS_E_INVALID_REQUEST;
    }

    if ((ret =
	 asn1_read_value(cert->cert, "tbsCertificate.serialNumber", result,
			 result_size)) < 0) {
	gnutls_assert();
	return _gnutls_asn2err(ret);
    }

    return 0;
}