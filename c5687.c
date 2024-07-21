ldns_rdf_new_frm_str(ldns_rdf_type type, const char *str)
{
	ldns_rdf *rdf = NULL;
	ldns_status status;

	switch (type) {
	case LDNS_RDF_TYPE_DNAME:
		status = ldns_str2rdf_dname(&rdf, str);
		break;
	case LDNS_RDF_TYPE_INT8:
		status = ldns_str2rdf_int8(&rdf, str);
		break;
	case LDNS_RDF_TYPE_INT16:
		status = ldns_str2rdf_int16(&rdf, str);
		break;
	case LDNS_RDF_TYPE_INT32:
		status = ldns_str2rdf_int32(&rdf, str);
		break;
	case LDNS_RDF_TYPE_A:
		status = ldns_str2rdf_a(&rdf, str);
		break;
	case LDNS_RDF_TYPE_AAAA:
		status = ldns_str2rdf_aaaa(&rdf, str);
		break;
	case LDNS_RDF_TYPE_STR:
		status = ldns_str2rdf_str(&rdf, str);
		break;
	case LDNS_RDF_TYPE_APL:
		status = ldns_str2rdf_apl(&rdf, str);
		break;
	case LDNS_RDF_TYPE_B64:
		status = ldns_str2rdf_b64(&rdf, str);
		break;
	case LDNS_RDF_TYPE_B32_EXT:
		status = ldns_str2rdf_b32_ext(&rdf, str);
		break;
	case LDNS_RDF_TYPE_HEX:
		status = ldns_str2rdf_hex(&rdf, str);
		break;
	case LDNS_RDF_TYPE_NSEC:
		status = ldns_str2rdf_nsec(&rdf, str);
		break;
	case LDNS_RDF_TYPE_TYPE:
		status = ldns_str2rdf_type(&rdf, str);
		break;
	case LDNS_RDF_TYPE_CLASS:
		status = ldns_str2rdf_class(&rdf, str);
		break;
	case LDNS_RDF_TYPE_CERT_ALG:
		status = ldns_str2rdf_cert_alg(&rdf, str);
		break;
	case LDNS_RDF_TYPE_ALG:
		status = ldns_str2rdf_alg(&rdf, str);
		break;
	case LDNS_RDF_TYPE_UNKNOWN:
		status = ldns_str2rdf_unknown(&rdf, str);
		break;
	case LDNS_RDF_TYPE_TIME:
		status = ldns_str2rdf_time(&rdf, str);
		break;
	case LDNS_RDF_TYPE_PERIOD:
		status = ldns_str2rdf_period(&rdf, str);
		break;
	case LDNS_RDF_TYPE_TSIG:
		status = ldns_str2rdf_tsig(&rdf, str);
		break;
	case LDNS_RDF_TYPE_SERVICE:
		status = ldns_str2rdf_service(&rdf, str);
		break;
	case LDNS_RDF_TYPE_LOC:
		status = ldns_str2rdf_loc(&rdf, str);
		break;
	case LDNS_RDF_TYPE_WKS:
		status = ldns_str2rdf_wks(&rdf, str);
		break;
	case LDNS_RDF_TYPE_NSAP:
		status = ldns_str2rdf_nsap(&rdf, str);
		break;
	case LDNS_RDF_TYPE_ATMA:
		status = ldns_str2rdf_atma(&rdf, str);
		break;
	case LDNS_RDF_TYPE_IPSECKEY:
		status = ldns_str2rdf_ipseckey(&rdf, str);
		break;
	case LDNS_RDF_TYPE_NSEC3_SALT:
		status = ldns_str2rdf_nsec3_salt(&rdf, str);
		break;
	case LDNS_RDF_TYPE_NSEC3_NEXT_OWNER:
		status = ldns_str2rdf_b32_ext(&rdf, str);
		break;
	case LDNS_RDF_TYPE_ILNP64:
		status = ldns_str2rdf_ilnp64(&rdf, str);
		break;
	case LDNS_RDF_TYPE_EUI48:
		status = ldns_str2rdf_eui48(&rdf, str);
		break;
	case LDNS_RDF_TYPE_EUI64:
		status = ldns_str2rdf_eui64(&rdf, str);
		break;
	case LDNS_RDF_TYPE_NONE:
	default:
		/* default default ??? */
		status = LDNS_STATUS_ERR;
		break;
	}
	if (LDNS_STATUS_OK == status) {
		ldns_rdf_set_type(rdf, type);
		return rdf;
	}
	if (rdf) {
		LDNS_FREE(rdf);
	}
	return NULL;
}