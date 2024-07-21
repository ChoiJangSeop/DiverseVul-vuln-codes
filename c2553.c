static encodePtr get_array_type(xmlNodePtr node, zval *array, smart_str *type TSRMLS_DC)
{
	HashTable *ht;
	int i, count, cur_type, prev_type, different;
	zval **tmp;
	char *prev_stype = NULL, *cur_stype = NULL, *prev_ns = NULL, *cur_ns = NULL;

	if (!array || Z_TYPE_P(array) != IS_ARRAY) {
		smart_str_appendl(type, "xsd:anyType", sizeof("xsd:anyType")-1);
		return get_conversion(XSD_ANYTYPE);
	}

	different = FALSE;
	cur_type = prev_type = 0;
	ht = HASH_OF(array);
	count = zend_hash_num_elements(ht);

	zend_hash_internal_pointer_reset(ht);
	for (i = 0;i < count;i++) {
		zend_hash_get_current_data(ht, (void **)&tmp);

		if (Z_TYPE_PP(tmp) == IS_OBJECT &&
		    Z_OBJCE_PP(tmp) == soap_var_class_entry) {
			zval **ztype;

			if (zend_hash_find(Z_OBJPROP_PP(tmp), "enc_type", sizeof("enc_type"), (void **)&ztype) == FAILURE) {
				soap_error0(E_ERROR,  "Encoding: SoapVar has no 'enc_type' property");
			}
			cur_type = Z_LVAL_PP(ztype);

			if (zend_hash_find(Z_OBJPROP_PP(tmp), "enc_stype", sizeof("enc_stype"), (void **)&ztype) == SUCCESS) {
				cur_stype = Z_STRVAL_PP(ztype);
			} else {
				cur_stype = NULL;
			}

			if (zend_hash_find(Z_OBJPROP_PP(tmp), "enc_ns", sizeof("enc_ns"), (void **)&ztype) == SUCCESS) {
				cur_ns = Z_STRVAL_PP(ztype);
			} else {
				cur_ns = NULL;
			}

		} else if (Z_TYPE_PP(tmp) == IS_ARRAY && is_map(*tmp)) {
			cur_type = APACHE_MAP;
			cur_stype = NULL;
			cur_ns = NULL;
		} else {
			cur_type = Z_TYPE_PP(tmp);
			cur_stype = NULL;
			cur_ns = NULL;
		}

		if (i > 0) {
			if ((cur_type != prev_type) ||
			    (cur_stype != NULL && prev_stype != NULL && strcmp(cur_stype,prev_stype) != 0) ||
			    (cur_stype == NULL && cur_stype != prev_stype) ||
			    (cur_ns != NULL && prev_ns != NULL && strcmp(cur_ns,prev_ns) != 0) ||
			    (cur_ns == NULL && cur_ns != prev_ns)) {
				different = TRUE;
				break;
			}
		}

		prev_type = cur_type;
		prev_stype = cur_stype;
		prev_ns = cur_ns;
		zend_hash_move_forward(ht);
	}

	if (different || count == 0) {
		smart_str_appendl(type, "xsd:anyType", sizeof("xsd:anyType")-1);
		return get_conversion(XSD_ANYTYPE);
	} else {
		encodePtr enc;

		if (cur_stype != NULL) {
			smart_str array_type = {0};

			if (cur_ns) {
				xmlNsPtr ns = encode_add_ns(node, cur_ns);

				smart_str_appends(type, (char*)ns->prefix);
				smart_str_appendc(type, ':');
				smart_str_appends(&array_type, cur_ns);
				smart_str_appendc(&array_type, ':');
			}
			smart_str_appends(type, cur_stype);
			smart_str_0(type);
			smart_str_appends(&array_type, cur_stype);
			smart_str_0(&array_type);

			enc = get_encoder_ex(SOAP_GLOBAL(sdl), array_type.c, array_type.len);
			smart_str_free(&array_type);
			return enc;
		} else {
			enc = get_conversion(cur_type);
			get_type_str(node, enc->details.ns, enc->details.type_str, type);
			return enc;
		}
	}
}