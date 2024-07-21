ldns_rdf2buffer_str(ldns_buffer *buffer, const ldns_rdf *rdf)
{
	ldns_status res = LDNS_STATUS_OK;

	/*ldns_buffer_printf(buffer, "%u:", ldns_rdf_get_type(rdf));*/
	if (rdf) {
		switch(ldns_rdf_get_type(rdf)) {
		case LDNS_RDF_TYPE_NONE:
			break;
		case LDNS_RDF_TYPE_DNAME:
			res = ldns_rdf2buffer_str_dname(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_INT8:
			res = ldns_rdf2buffer_str_int8(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_INT16:
			res = ldns_rdf2buffer_str_int16(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_INT32:
			res = ldns_rdf2buffer_str_int32(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_PERIOD:
			res = ldns_rdf2buffer_str_period(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_TSIGTIME:
			res = ldns_rdf2buffer_str_tsigtime(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_A:
			res = ldns_rdf2buffer_str_a(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_AAAA:
			res = ldns_rdf2buffer_str_aaaa(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_STR:
			res = ldns_rdf2buffer_str_str(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_APL:
			res = ldns_rdf2buffer_str_apl(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_B32_EXT:
			res = ldns_rdf2buffer_str_b32_ext(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_B64:
			res = ldns_rdf2buffer_str_b64(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_HEX:
			res = ldns_rdf2buffer_str_hex(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_NSEC:
			res = ldns_rdf2buffer_str_nsec(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_NSEC3_SALT:
			res = ldns_rdf2buffer_str_nsec3_salt(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_TYPE:
			res = ldns_rdf2buffer_str_type(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_CLASS:
			res = ldns_rdf2buffer_str_class(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_CERT_ALG:
			res = ldns_rdf2buffer_str_cert_alg(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_ALG:
			res = ldns_rdf2buffer_str_alg(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_UNKNOWN:
			res = ldns_rdf2buffer_str_unknown(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_TIME:
			res = ldns_rdf2buffer_str_time(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_LOC:
			res = ldns_rdf2buffer_str_loc(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_WKS:
		case LDNS_RDF_TYPE_SERVICE:
			res = ldns_rdf2buffer_str_wks(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_NSAP:
			res = ldns_rdf2buffer_str_nsap(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_ATMA:
			res = ldns_rdf2buffer_str_atma(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_IPSECKEY:
			res = ldns_rdf2buffer_str_ipseckey(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_TSIG:
			res = ldns_rdf2buffer_str_tsig(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_INT16_DATA:
			res = ldns_rdf2buffer_str_int16_data(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_NSEC3_NEXT_OWNER:
			res = ldns_rdf2buffer_str_b32_ext(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_ILNP64:
			res = ldns_rdf2buffer_str_ilnp64(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_EUI48:
			res = ldns_rdf2buffer_str_eui48(buffer, rdf);
			break;
		case LDNS_RDF_TYPE_EUI64:
			res = ldns_rdf2buffer_str_eui64(buffer, rdf);
			break;
		}
	} else {
		/** This will write mangled RRs */
		ldns_buffer_printf(buffer, "(null) ");
		res = LDNS_STATUS_ERR;
	}
	return res;
}