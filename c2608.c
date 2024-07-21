int gnutls_x509_ext_import_crl_dist_points(const gnutls_datum_t * ext,
					gnutls_x509_crl_dist_points_t cdp,
					unsigned int flags)
{
	int result;
	ASN1_TYPE c2 = ASN1_TYPE_EMPTY;
	char name[ASN1_MAX_NAME_SIZE];
	int len, ret;
	uint8_t reasons[2];
	unsigned i, type, rflags, j;
	gnutls_datum_t san;

	result = asn1_create_element
	    (_gnutls_get_pkix(), "PKIX1.CRLDistributionPoints", &c2);
	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		return _gnutls_asn2err(result);
	}

	result = asn1_der_decoding(&c2, ext->data, ext->size, NULL);

	if (result != ASN1_SUCCESS) {
		gnutls_assert();
		ret = _gnutls_asn2err(result);
		goto cleanup;
	}

	/* Return the different names from the first CRLDistr. point.
	 * The whole thing is a mess.
	 */

	i = 0;
	do {
		san.data = NULL;
		san.size = 0;

		snprintf(name, sizeof(name), "?%u.reasons", (unsigned)i + 1);

		len = sizeof(reasons);
		result = asn1_read_value(c2, name, reasons, &len);

		if (result != ASN1_VALUE_NOT_FOUND &&
		    result != ASN1_ELEMENT_NOT_FOUND &&
		    result != ASN1_SUCCESS) {
			gnutls_assert();
			ret = _gnutls_asn2err(result);
			break;
		}

		if (result == ASN1_VALUE_NOT_FOUND
		    || result == ASN1_ELEMENT_NOT_FOUND)
			rflags = 0;
		else
			rflags = reasons[0] | (reasons[1] << 8);

		snprintf(name, sizeof(name),
			 "?%u.distributionPoint.fullName", (unsigned)i + 1);

		j = 0;
		do {
			ret =
			    _gnutls_parse_general_name2(c2, name, j, &san,
							&type, 0);
			if (j > 0
			    && ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
				ret = 0;
				break;
			}
			if (ret < 0)
				break;

			ret = crl_dist_points_set(cdp, type, &san, rflags);
			if (ret < 0)
				break;

			j++;
		} while (ret >= 0);

		i++;
	} while (ret >= 0);

	if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
		gnutls_assert();
		goto cleanup;
	}

	ret = 0;
 cleanup:
	asn1_delete_structure(&c2);
	return ret;
}