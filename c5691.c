ldns_wire2rdf(ldns_rr *rr, const uint8_t *wire, size_t max, size_t *pos)
{
	size_t end;
	size_t cur_rdf_length;
	uint8_t rdf_index;
	uint8_t *data;
	uint16_t rd_length;
	ldns_rdf *cur_rdf = NULL;
	ldns_rdf_type cur_rdf_type;
	const ldns_rr_descriptor *descriptor = ldns_rr_descript(ldns_rr_get_type(rr));
	ldns_status status;

	if (*pos + 2 > max) {
		return LDNS_STATUS_PACKET_OVERFLOW;
	}

	rd_length = ldns_read_uint16(&wire[*pos]);
	*pos = *pos + 2;

	if (*pos + rd_length > max) {
		return LDNS_STATUS_PACKET_OVERFLOW;
	}

	end = *pos + (size_t) rd_length;

	for (rdf_index = 0;
	     rdf_index < ldns_rr_descriptor_maximum(descriptor); rdf_index++) {
		if (*pos >= end) {
			break;
		}
		cur_rdf_length = 0;

		cur_rdf_type = ldns_rr_descriptor_field_type(descriptor, rdf_index);
		/* handle special cases immediately, set length
		   for fixed length rdata and do them below */
		switch (cur_rdf_type) {
		case LDNS_RDF_TYPE_DNAME:
			status = ldns_wire2dname(&cur_rdf, wire, max, pos);
			LDNS_STATUS_CHECK_RETURN(status);
			break;
		case LDNS_RDF_TYPE_CLASS:
		case LDNS_RDF_TYPE_ALG:
		case LDNS_RDF_TYPE_INT8:
			cur_rdf_length = LDNS_RDF_SIZE_BYTE;
			break;
		case LDNS_RDF_TYPE_TYPE:
		case LDNS_RDF_TYPE_INT16:
		case LDNS_RDF_TYPE_CERT_ALG:
			cur_rdf_length = LDNS_RDF_SIZE_WORD;
			break;
		case LDNS_RDF_TYPE_TIME:
		case LDNS_RDF_TYPE_INT32:
		case LDNS_RDF_TYPE_A:
		case LDNS_RDF_TYPE_PERIOD:
			cur_rdf_length = LDNS_RDF_SIZE_DOUBLEWORD;
			break;
		case LDNS_RDF_TYPE_TSIGTIME:
		case LDNS_RDF_TYPE_EUI48:
			cur_rdf_length = LDNS_RDF_SIZE_6BYTES;
			break;
		case LDNS_RDF_TYPE_ILNP64:
		case LDNS_RDF_TYPE_EUI64:
			cur_rdf_length = LDNS_RDF_SIZE_8BYTES;
			break;
		case LDNS_RDF_TYPE_AAAA:
			cur_rdf_length = LDNS_RDF_SIZE_16BYTES;
			break;
		case LDNS_RDF_TYPE_STR:
		case LDNS_RDF_TYPE_NSEC3_SALT:
			/* len is stored in first byte
			 * it should be in the rdf too, so just
			 * copy len+1 from this position
			 */
			cur_rdf_length = ((size_t) wire[*pos]) + 1;
			break;
		case LDNS_RDF_TYPE_INT16_DATA:
			cur_rdf_length = (size_t) ldns_read_uint16(&wire[*pos]) + 2;
			break;
		case LDNS_RDF_TYPE_B32_EXT:
		case LDNS_RDF_TYPE_NSEC3_NEXT_OWNER:
			/* length is stored in first byte */
			cur_rdf_length = ((size_t) wire[*pos]) + 1;
			break;
		case LDNS_RDF_TYPE_APL:
		case LDNS_RDF_TYPE_B64:
		case LDNS_RDF_TYPE_HEX:
		case LDNS_RDF_TYPE_NSEC:
		case LDNS_RDF_TYPE_UNKNOWN:
		case LDNS_RDF_TYPE_SERVICE:
		case LDNS_RDF_TYPE_LOC:
		case LDNS_RDF_TYPE_WKS:
		case LDNS_RDF_TYPE_NSAP:
		case LDNS_RDF_TYPE_ATMA:
		case LDNS_RDF_TYPE_IPSECKEY:
		case LDNS_RDF_TYPE_TSIG:
		case LDNS_RDF_TYPE_NONE:
			/*
			 * Read to end of rr rdata
			 */
			cur_rdf_length = end - *pos;
			break;
		}

		/* fixed length rdata */
		if (cur_rdf_length > 0) {
			if (cur_rdf_length + *pos > end) {
				return LDNS_STATUS_PACKET_OVERFLOW;
			}
			data = LDNS_XMALLOC(uint8_t, rd_length);
			if (!data) {
				return LDNS_STATUS_MEM_ERR;
			}
			memcpy(data, &wire[*pos], cur_rdf_length);

			cur_rdf = ldns_rdf_new(cur_rdf_type, cur_rdf_length, data);
			*pos = *pos + cur_rdf_length;
		}

		if (cur_rdf) {
			ldns_rr_push_rdf(rr, cur_rdf);
			cur_rdf = NULL;
		}
	}

	return LDNS_STATUS_OK;
}