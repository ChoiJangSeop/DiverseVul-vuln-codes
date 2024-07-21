static void php_wddx_serialize_array(wddx_packet *packet, zval *arr)
{
	zval **ent;
	char *key;
	uint key_len;
	int is_struct = 0, ent_type;
	ulong idx;
	HashTable *target_hash;
	char tmp_buf[WDDX_BUF_LEN];
	ulong ind = 0;
	int type;
	TSRMLS_FETCH();

	target_hash = HASH_OF(arr);

	for (zend_hash_internal_pointer_reset(target_hash);
		 zend_hash_get_current_data(target_hash, (void**)&ent) == SUCCESS;
		 zend_hash_move_forward(target_hash)) {

		type = zend_hash_get_current_key(target_hash, &key, &idx, 0);

		if (type == HASH_KEY_IS_STRING) {
			is_struct = 1;
			break;
		}

		if (idx != ind) {
			is_struct = 1;
			break;
		}

		ind++;
	}

	if (is_struct) {
		php_wddx_add_chunk_static(packet, WDDX_STRUCT_S);
	} else {
		snprintf(tmp_buf, sizeof(tmp_buf), WDDX_ARRAY_S, zend_hash_num_elements(target_hash));
		php_wddx_add_chunk(packet, tmp_buf);
	}

	for (zend_hash_internal_pointer_reset(target_hash);
		 zend_hash_get_current_data(target_hash, (void**)&ent) == SUCCESS;
		 zend_hash_move_forward(target_hash)) {
		if (*ent == arr) {
			continue;
		}

		if (is_struct) {
			ent_type = zend_hash_get_current_key_ex(target_hash, &key, &key_len, &idx, 0, NULL);

			if (ent_type == HASH_KEY_IS_STRING) {
				php_wddx_serialize_var(packet, *ent, key, key_len TSRMLS_CC);
			} else {
				key_len = slprintf(tmp_buf, sizeof(tmp_buf), "%ld", idx);
				php_wddx_serialize_var(packet, *ent, tmp_buf, key_len TSRMLS_CC);
			}
		} else {
			php_wddx_serialize_var(packet, *ent, NULL, 0 TSRMLS_CC);
		}
	}
	
	if (is_struct) {
		php_wddx_add_chunk_static(packet, WDDX_STRUCT_E);
	} else {
		php_wddx_add_chunk_static(packet, WDDX_ARRAY_E);
	}
}