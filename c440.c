static int encode_to_private_key_info(gnutls_x509_privkey_t pkey,
				      gnutls_datum_t * der,
				      ASN1_TYPE * pkey_info)
{
    int result;
    size_t size;
    opaque *data = NULL;
    opaque null = 0;

    if (pkey->pk_algorithm != GNUTLS_PK_RSA) {
	gnutls_assert();
	return GNUTLS_E_UNIMPLEMENTED_FEATURE;
    }

    if ((result =
	 asn1_create_element(_gnutls_get_pkix(),
			     "PKIX1.pkcs-8-PrivateKeyInfo",
			     pkey_info)) != ASN1_SUCCESS) {
	gnutls_assert();
	result = _gnutls_asn2err(result);
	goto error;
    }

    /* Write the version.
     */
    result = asn1_write_value(*pkey_info, "version", &null, 1);
    if (result != ASN1_SUCCESS) {
	gnutls_assert();
	result = _gnutls_asn2err(result);
	goto error;
    }

    /* write the privateKeyAlgorithm
     * fields. (OID+NULL data)
     */
    result =
	asn1_write_value(*pkey_info, "privateKeyAlgorithm.algorithm",
			 PKIX1_RSA_OID, 1);
    if (result != ASN1_SUCCESS) {
	gnutls_assert();
	result = _gnutls_asn2err(result);
	goto error;
    }

    result =
	asn1_write_value(*pkey_info, "privateKeyAlgorithm.parameters",
			 NULL, 0);
    if (result != ASN1_SUCCESS) {
	gnutls_assert();
	result = _gnutls_asn2err(result);
	goto error;
    }

    /* Write the raw private key
     */
    size = 0;
    result =
	gnutls_x509_privkey_export(pkey, GNUTLS_X509_FMT_DER, NULL, &size);
    if (result != GNUTLS_E_SHORT_MEMORY_BUFFER) {
	gnutls_assert();
	goto error;
    }

    data = gnutls_alloca(size);
    if (data == NULL) {
	gnutls_assert();
	result = GNUTLS_E_MEMORY_ERROR;
	goto error;
    }


    result =
	gnutls_x509_privkey_export(pkey, GNUTLS_X509_FMT_DER, data, &size);
    if (result < 0) {
	gnutls_assert();
	goto error;
    }

    result = asn1_write_value(*pkey_info, "privateKey", data, size);

    gnutls_afree(data);
    data = NULL;

    if (result != ASN1_SUCCESS) {
	gnutls_assert();
	result = _gnutls_asn2err(result);
	goto error;
    }

    /* Append an empty Attributes field.
     */
    result = asn1_write_value(*pkey_info, "attributes", NULL, 0);
    if (result != ASN1_SUCCESS) {
	gnutls_assert();
	result = _gnutls_asn2err(result);
	goto error;
    }

    /* DER Encode the generated private key info.
     */
    size = 0;
    result = asn1_der_coding(*pkey_info, "", NULL, &size, NULL);
    if (result != ASN1_MEM_ERROR) {
	gnutls_assert();
	result = _gnutls_asn2err(result);
	goto error;
    }

    /* allocate data for the der
     */
    der->size = size;
    der->data = gnutls_malloc(size);
    if (der->data == NULL) {
	gnutls_assert();
	return GNUTLS_E_MEMORY_ERROR;
    }

    result = asn1_der_coding(*pkey_info, "", der->data, &size, NULL);
    if (result != ASN1_SUCCESS) {
	gnutls_assert();
	result = _gnutls_asn2err(result);
	goto error;
    }

    return 0;

  error:
    asn1_delete_structure(pkey_info);
    if (data != NULL) {
	gnutls_afree(data);
    }
    return result;

}