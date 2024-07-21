_PUBLIC_ NTSTATUS ldap_decode(struct asn1_data *data,
			      const struct ldap_control_handler *control_handlers,
			      struct ldap_message *msg)
{
	uint8_t tag;

	asn1_start_tag(data, ASN1_SEQUENCE(0));
	asn1_read_Integer(data, &msg->messageid);

	if (!asn1_peek_uint8(data, &tag))
		return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);

	switch(tag) {

	case ASN1_APPLICATION(LDAP_TAG_BindRequest): {
		struct ldap_BindRequest *r = &msg->r.BindRequest;
		msg->type = LDAP_TAG_BindRequest;
		asn1_start_tag(data, tag);
		asn1_read_Integer(data, &r->version);
		asn1_read_OctetString_talloc(msg, data, &r->dn);
		if (asn1_peek_tag(data, ASN1_CONTEXT_SIMPLE(0))) {
			int pwlen;
			r->creds.password = "";
			r->mechanism = LDAP_AUTH_MECH_SIMPLE;
			asn1_start_tag(data, ASN1_CONTEXT_SIMPLE(0));
			pwlen = asn1_tag_remaining(data);
			if (pwlen == -1) {
				return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
			}
			if (pwlen != 0) {
				char *pw = talloc_array(msg, char, pwlen+1);
				if (!pw) {
					return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
				}
				asn1_read(data, pw, pwlen);
				pw[pwlen] = '\0';
				r->creds.password = pw;
			}
			asn1_end_tag(data);
		} else if (asn1_peek_tag(data, ASN1_CONTEXT(3))){
			asn1_start_tag(data, ASN1_CONTEXT(3));
			r->mechanism = LDAP_AUTH_MECH_SASL;
			asn1_read_OctetString_talloc(msg, data, &r->creds.SASL.mechanism);
			if (asn1_peek_tag(data, ASN1_OCTET_STRING)) { /* optional */
				DATA_BLOB tmp_blob = data_blob(NULL, 0);
				asn1_read_OctetString(data, msg, &tmp_blob);
				r->creds.SASL.secblob = talloc(msg, DATA_BLOB);
				if (!r->creds.SASL.secblob) {
					return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
				}
				*r->creds.SASL.secblob = data_blob_talloc(r->creds.SASL.secblob,
									  tmp_blob.data, tmp_blob.length);
				data_blob_free(&tmp_blob);
			} else {
				r->creds.SASL.secblob = NULL;
			}
			asn1_end_tag(data);
		} else {
			/* Neither Simple nor SASL bind */
			return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
		}
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_BindResponse): {
		struct ldap_BindResponse *r = &msg->r.BindResponse;
		msg->type = LDAP_TAG_BindResponse;
		asn1_start_tag(data, tag);
		ldap_decode_response(msg, data, &r->response);
		if (asn1_peek_tag(data, ASN1_CONTEXT_SIMPLE(7))) {
			DATA_BLOB tmp_blob = data_blob(NULL, 0);
			asn1_read_ContextSimple(data, 7, &tmp_blob);
			r->SASL.secblob = talloc(msg, DATA_BLOB);
			if (!r->SASL.secblob) {
				return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
			}
			*r->SASL.secblob = data_blob_talloc(r->SASL.secblob,
							    tmp_blob.data, tmp_blob.length);
			data_blob_free(&tmp_blob);
		} else {
			r->SASL.secblob = NULL;
		}
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION_SIMPLE(LDAP_TAG_UnbindRequest): {
		msg->type = LDAP_TAG_UnbindRequest;
		asn1_start_tag(data, tag);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_SearchRequest): {
		struct ldap_SearchRequest *r = &msg->r.SearchRequest;
		int sizelimit, timelimit;
		const char **attrs = NULL;
		msg->type = LDAP_TAG_SearchRequest;
		asn1_start_tag(data, tag);
		asn1_read_OctetString_talloc(msg, data, &r->basedn);
		asn1_read_enumerated(data, (int *)(void *)&(r->scope));
		asn1_read_enumerated(data, (int *)(void *)&(r->deref));
		asn1_read_Integer(data, &sizelimit);
		r->sizelimit = sizelimit;
		asn1_read_Integer(data, &timelimit);
		r->timelimit = timelimit;
		asn1_read_BOOLEAN(data, &r->attributesonly);

		r->tree = ldap_decode_filter_tree(msg, data);
		if (r->tree == NULL) {
			return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
		}

		asn1_start_tag(data, ASN1_SEQUENCE(0));

		r->num_attributes = 0;
		r->attributes = NULL;

		while (asn1_tag_remaining(data) > 0) {					

			const char *attr;
			if (!asn1_read_OctetString_talloc(msg, data,
							  &attr))
				return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
			if (!add_string_to_array(msg, attr,
						 &attrs,
						 &r->num_attributes))
				return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
		}
		r->attributes = attrs;

		asn1_end_tag(data);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_SearchResultEntry): {
		struct ldap_SearchResEntry *r = &msg->r.SearchResultEntry;
		msg->type = LDAP_TAG_SearchResultEntry;
		r->attributes = NULL;
		r->num_attributes = 0;
		asn1_start_tag(data, tag);
		asn1_read_OctetString_talloc(msg, data, &r->dn);
		ldap_decode_attribs(msg, data, &r->attributes,
				    &r->num_attributes);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_SearchResultDone): {
		struct ldap_Result *r = &msg->r.SearchResultDone;
		msg->type = LDAP_TAG_SearchResultDone;
		asn1_start_tag(data, tag);
		ldap_decode_response(msg, data, r);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_SearchResultReference): {
		struct ldap_SearchResRef *r = &msg->r.SearchResultReference;
		msg->type = LDAP_TAG_SearchResultReference;
		asn1_start_tag(data, tag);
		asn1_read_OctetString_talloc(msg, data, &r->referral);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_ModifyRequest): {
		struct ldap_ModifyRequest *r = &msg->r.ModifyRequest;
		msg->type = LDAP_TAG_ModifyRequest;
		asn1_start_tag(data, ASN1_APPLICATION(LDAP_TAG_ModifyRequest));
		asn1_read_OctetString_talloc(msg, data, &r->dn);
		asn1_start_tag(data, ASN1_SEQUENCE(0));

		r->num_mods = 0;
		r->mods = NULL;

		while (asn1_tag_remaining(data) > 0) {
			struct ldap_mod mod;
			int v;
			ZERO_STRUCT(mod);
			asn1_start_tag(data, ASN1_SEQUENCE(0));
			asn1_read_enumerated(data, &v);
			mod.type = v;
			ldap_decode_attrib(msg, data, &mod.attrib);
			asn1_end_tag(data);
			if (!add_mod_to_array_talloc(msg, &mod,
						     &r->mods, &r->num_mods)) {
				return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
			}
		}

		asn1_end_tag(data);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_ModifyResponse): {
		struct ldap_Result *r = &msg->r.ModifyResponse;
		msg->type = LDAP_TAG_ModifyResponse;
		asn1_start_tag(data, tag);
		ldap_decode_response(msg, data, r);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_AddRequest): {
		struct ldap_AddRequest *r = &msg->r.AddRequest;
		msg->type = LDAP_TAG_AddRequest;
		asn1_start_tag(data, tag);
		asn1_read_OctetString_talloc(msg, data, &r->dn);

		r->attributes = NULL;
		r->num_attributes = 0;
		ldap_decode_attribs(msg, data, &r->attributes,
				    &r->num_attributes);

		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_AddResponse): {
		struct ldap_Result *r = &msg->r.AddResponse;
		msg->type = LDAP_TAG_AddResponse;
		asn1_start_tag(data, tag);
		ldap_decode_response(msg, data, r);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION_SIMPLE(LDAP_TAG_DelRequest): {
		struct ldap_DelRequest *r = &msg->r.DelRequest;
		int len;
		char *dn;
		msg->type = LDAP_TAG_DelRequest;
		asn1_start_tag(data,
			       ASN1_APPLICATION_SIMPLE(LDAP_TAG_DelRequest));
		len = asn1_tag_remaining(data);
		if (len == -1) {
			return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
		}
		dn = talloc_array(msg, char, len+1);
		if (dn == NULL)
			break;
		asn1_read(data, dn, len);
		dn[len] = '\0';
		r->dn = dn;
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_DelResponse): {
		struct ldap_Result *r = &msg->r.DelResponse;
		msg->type = LDAP_TAG_DelResponse;
		asn1_start_tag(data, tag);
		ldap_decode_response(msg, data, r);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_ModifyDNRequest): {
		struct ldap_ModifyDNRequest *r = &msg->r.ModifyDNRequest;
		msg->type = LDAP_TAG_ModifyDNRequest;
		asn1_start_tag(data,
			       ASN1_APPLICATION(LDAP_TAG_ModifyDNRequest));
		asn1_read_OctetString_talloc(msg, data, &r->dn);
		asn1_read_OctetString_talloc(msg, data, &r->newrdn);
		asn1_read_BOOLEAN(data, &r->deleteolddn);
		r->newsuperior = NULL;
		if (asn1_tag_remaining(data) > 0) {
			int len;
			char *newsup;
			asn1_start_tag(data, ASN1_CONTEXT_SIMPLE(0));
			len = asn1_tag_remaining(data);
			if (len == -1) {
				return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
			}
			newsup = talloc_array(msg, char, len+1);
			if (newsup == NULL) {
				return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
			}
			asn1_read(data, newsup, len);
			newsup[len] = '\0';
			r->newsuperior = newsup;
			asn1_end_tag(data);
		}
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_ModifyDNResponse): {
		struct ldap_Result *r = &msg->r.ModifyDNResponse;
		msg->type = LDAP_TAG_ModifyDNResponse;
		asn1_start_tag(data, tag);
		ldap_decode_response(msg, data, r);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_CompareRequest): {
		struct ldap_CompareRequest *r = &msg->r.CompareRequest;
		msg->type = LDAP_TAG_CompareRequest;
		asn1_start_tag(data,
			       ASN1_APPLICATION(LDAP_TAG_CompareRequest));
		asn1_read_OctetString_talloc(msg, data, &r->dn);
		asn1_start_tag(data, ASN1_SEQUENCE(0));
		asn1_read_OctetString_talloc(msg, data, &r->attribute);
		asn1_read_OctetString(data, msg, &r->value);
		if (r->value.data) {
			talloc_steal(msg, r->value.data);
		}
		asn1_end_tag(data);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_CompareResponse): {
		struct ldap_Result *r = &msg->r.CompareResponse;
		msg->type = LDAP_TAG_CompareResponse;
		asn1_start_tag(data, tag);
		ldap_decode_response(msg, data, r);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION_SIMPLE(LDAP_TAG_AbandonRequest): {
		struct ldap_AbandonRequest *r = &msg->r.AbandonRequest;
		msg->type = LDAP_TAG_AbandonRequest;
		asn1_start_tag(data, tag);
		asn1_read_implicit_Integer(data, &r->messageid);
		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_ExtendedRequest): {
		struct ldap_ExtendedRequest *r = &msg->r.ExtendedRequest;
		DATA_BLOB tmp_blob = data_blob(NULL, 0);

		msg->type = LDAP_TAG_ExtendedRequest;
		asn1_start_tag(data,tag);
		if (!asn1_read_ContextSimple(data, 0, &tmp_blob)) {
			return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
		}
		r->oid = blob2string_talloc(msg, tmp_blob);
		data_blob_free(&tmp_blob);
		if (!r->oid) {
			return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
		}

		if (asn1_peek_tag(data, ASN1_CONTEXT_SIMPLE(1))) {
			asn1_read_ContextSimple(data, 1, &tmp_blob);
			r->value = talloc(msg, DATA_BLOB);
			if (!r->value) {
				return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
			}
			*r->value = data_blob_talloc(r->value, tmp_blob.data, tmp_blob.length);
			data_blob_free(&tmp_blob);
		} else {
			r->value = NULL;
		}

		asn1_end_tag(data);
		break;
	}

	case ASN1_APPLICATION(LDAP_TAG_ExtendedResponse): {
		struct ldap_ExtendedResponse *r = &msg->r.ExtendedResponse;
		DATA_BLOB tmp_blob = data_blob(NULL, 0);

		msg->type = LDAP_TAG_ExtendedResponse;
		asn1_start_tag(data, tag);		
		ldap_decode_response(msg, data, &r->response);

		if (asn1_peek_tag(data, ASN1_CONTEXT_SIMPLE(10))) {
			asn1_read_ContextSimple(data, 1, &tmp_blob);
			r->oid = blob2string_talloc(msg, tmp_blob);
			data_blob_free(&tmp_blob);
			if (!r->oid) {
				return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
			}
		} else {
			r->oid = NULL;
		}

		if (asn1_peek_tag(data, ASN1_CONTEXT_SIMPLE(11))) {
			asn1_read_ContextSimple(data, 1, &tmp_blob);
			r->value = talloc(msg, DATA_BLOB);
			if (!r->value) {
				return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
			}
			*r->value = data_blob_talloc(r->value, tmp_blob.data, tmp_blob.length);
			data_blob_free(&tmp_blob);
		} else {
			r->value = NULL;
		}

		asn1_end_tag(data);
		break;
	}
	default: 
		return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
	}

	msg->controls = NULL;
	msg->controls_decoded = NULL;

	if (asn1_peek_tag(data, ASN1_CONTEXT(0))) {
		int i = 0;
		struct ldb_control **ctrl = NULL;
		bool *decoded = NULL;

		asn1_start_tag(data, ASN1_CONTEXT(0));

		while (asn1_peek_tag(data, ASN1_SEQUENCE(0))) {
			DATA_BLOB value;
			/* asn1_start_tag(data, ASN1_SEQUENCE(0)); */

			ctrl = talloc_realloc(msg, ctrl, struct ldb_control *, i+2);
			if (!ctrl) {
				return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
			}

			decoded = talloc_realloc(msg, decoded, bool, i+1);
			if (!decoded) {
				return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
			}

			ctrl[i] = talloc(ctrl, struct ldb_control);
			if (!ctrl[i]) {
				return NT_STATUS_LDAP(LDAP_OPERATIONS_ERROR);
			}

			if (!ldap_decode_control_wrapper(ctrl[i], data, ctrl[i], &value)) {
				return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
			}
			
			if (!ldap_decode_control_value(ctrl[i], value,
						       control_handlers,
						       ctrl[i])) {
				if (ctrl[i]->critical) {
					ctrl[i]->data = NULL;
					decoded[i] = false;
					i++;
				} else {
					talloc_free(ctrl[i]);
					ctrl[i] = NULL;
				}
			} else {
				decoded[i] = true;
				i++;
			}
		}

		if (ctrl != NULL) {
			ctrl[i] = NULL;
		}

		msg->controls = ctrl;
		msg->controls_decoded = decoded;

		asn1_end_tag(data);
	}

	asn1_end_tag(data);
	if ((data->has_error) || (data->nesting != NULL)) {
		return NT_STATUS_LDAP(LDAP_PROTOCOL_ERROR);
	}
	return NT_STATUS_OK;
}