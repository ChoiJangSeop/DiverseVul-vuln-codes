_gnutls_x509_dn_to_string(const char *oid, void *value,
			  int value_size, gnutls_datum_t * str)
{
	const struct oid_to_string *oentry;
	int ret;
	gnutls_datum_t tmp;
	size_t size;

	if (value == NULL || value_size <= 0) {
		gnutls_assert();
		return GNUTLS_E_INVALID_REQUEST;
	}

	oentry = get_oid_entry(oid);
	if (oentry == NULL) {	/* unknown OID -> hex */
 unknown_oid:
		str->size = value_size * 2 + 2;
		str->data = gnutls_malloc(str->size);
		if (str->data == NULL)
			return gnutls_assert_val(GNUTLS_E_MEMORY_ERROR);

		size = str->size;
		ret = data2hex(value, value_size, str->data, &size);
		if (ret < 0) {
			gnutls_assert();
			gnutls_free(str->data);
			return ret;
		}
		str->size = size;
		return 0;
	}

	if (oentry->asn_desc != NULL) {	/* complex */
		ret =
		    decode_complex_string(oentry, value, value_size, &tmp);
		if (ret < 0) {
			/* we failed decoding -> handle it as unknown OID */
			goto unknown_oid;
		}
	} else {
		ret =
		    _gnutls_x509_decode_string(oentry->etype, value,
					       value_size, &tmp, 0);
		if (ret < 0) {
			/* we failed decoding -> handle it as unknown OID */
			goto unknown_oid;
		}
	}

	ret = str_escape(&tmp, str);
	_gnutls_free_datum(&tmp);

	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}