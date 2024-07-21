static HashTable* soap_create_typemap(sdlPtr sdl, HashTable *ht TSRMLS_DC)
{
	zval **tmp;
	HashTable *ht2;
	HashPosition pos1, pos2;
	HashTable *typemap = NULL;
	
	zend_hash_internal_pointer_reset_ex(ht, &pos1);
	while (zend_hash_get_current_data_ex(ht, (void**)&tmp, &pos1) == SUCCESS) {
		char *type_name = NULL;
		char *type_ns = NULL;
		zval *to_xml = NULL;
		zval *to_zval = NULL;
		encodePtr enc, new_enc;

		if (Z_TYPE_PP(tmp) != IS_ARRAY) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Wrong 'typemap' option");
			return NULL;
		}
		ht2 = Z_ARRVAL_PP(tmp);

		zend_hash_internal_pointer_reset_ex(ht2, &pos2);
		while (zend_hash_get_current_data_ex(ht2, (void**)&tmp, &pos2) == SUCCESS) {
			char *name = NULL;
			unsigned int name_len;
			ulong index;

			zend_hash_get_current_key_ex(ht2, &name, &name_len, &index, 0, &pos2);
			if (name) {
				if (name_len == sizeof("type_name") &&
				    strncmp(name, "type_name", sizeof("type_name")-1) == 0) {
					if (Z_TYPE_PP(tmp) == IS_STRING) {
						type_name = Z_STRVAL_PP(tmp);
					} else if (Z_TYPE_PP(tmp) != IS_NULL) {
					}
				} else if (name_len == sizeof("type_ns") &&
				    strncmp(name, "type_ns", sizeof("type_ns")-1) == 0) {
					if (Z_TYPE_PP(tmp) == IS_STRING) {
						type_ns = Z_STRVAL_PP(tmp);
					} else if (Z_TYPE_PP(tmp) != IS_NULL) {
					}
				} else if (name_len == sizeof("to_xml") &&
				    strncmp(name, "to_xml", sizeof("to_xml")-1) == 0) {
					to_xml = *tmp;
				} else if (name_len == sizeof("from_xml") &&
				    strncmp(name, "from_xml", sizeof("from_xml")-1) == 0) {
					to_zval = *tmp;
				}
			}
			zend_hash_move_forward_ex(ht2, &pos2);
		}		

		if (type_name) {
			smart_str nscat = {0};

			if (type_ns) {
				enc = get_encoder(sdl, type_ns, type_name);
			} else {
				enc = get_encoder_ex(sdl, type_name, strlen(type_name));
			}

			new_enc = emalloc(sizeof(encode));
			memset(new_enc, 0, sizeof(encode));

			if (enc) {
				new_enc->details.type = enc->details.type;
				new_enc->details.ns = estrdup(enc->details.ns);
				new_enc->details.type_str = estrdup(enc->details.type_str);
				new_enc->details.sdl_type = enc->details.sdl_type;
			} else {
				enc = get_conversion(UNKNOWN_TYPE);
				new_enc->details.type = enc->details.type;
				if (type_ns) {
					new_enc->details.ns = estrdup(type_ns);
				}
				new_enc->details.type_str = estrdup(type_name);
			}
			new_enc->to_xml = enc->to_xml;
			new_enc->to_zval = enc->to_zval;
			new_enc->details.map = emalloc(sizeof(soapMapping));
			memset(new_enc->details.map, 0, sizeof(soapMapping));			
			if (to_xml) {
				zval_add_ref(&to_xml);
				new_enc->details.map->to_xml = to_xml;
				new_enc->to_xml = to_xml_user;
			} else if (enc->details.map && enc->details.map->to_xml) {
				zval_add_ref(&enc->details.map->to_xml);
				new_enc->details.map->to_xml = enc->details.map->to_xml;
			}
			if (to_zval) {
				zval_add_ref(&to_zval);
				new_enc->details.map->to_zval = to_zval;
				new_enc->to_zval = to_zval_user;
			} else if (enc->details.map && enc->details.map->to_zval) {
				zval_add_ref(&enc->details.map->to_zval);
				new_enc->details.map->to_zval = enc->details.map->to_zval;
			}
			if (!typemap) {
				typemap = emalloc(sizeof(HashTable));
				zend_hash_init(typemap, 0, NULL, delete_encoder, 0);
			}

			if (type_ns) {
				smart_str_appends(&nscat, type_ns);
				smart_str_appendc(&nscat, ':');
			}
			smart_str_appends(&nscat, type_name);
			smart_str_0(&nscat);
			zend_hash_update(typemap, nscat.c, nscat.len + 1, &new_enc, sizeof(encodePtr), NULL);
			smart_str_free(&nscat);
		}
		zend_hash_move_forward_ex(ht, &pos1);
	}
	return typemap;
}