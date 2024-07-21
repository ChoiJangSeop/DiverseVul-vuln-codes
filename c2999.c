PHPAPI void var_destroy(php_unserialize_data_t *var_hashx)
{
	void *next;
	long i;
	var_entries *var_hash = (*var_hashx)->first;
#if VAR_ENTRIES_DBG
	fprintf(stderr, "var_destroy(%ld)\n", var_hash?var_hash->used_slots:-1L);
#endif
	
	while (var_hash) {
		next = var_hash->next;
		efree(var_hash);
		var_hash = next;
	}

	var_hash = (*var_hashx)->first_dtor;
	
	while (var_hash) {
		for (i = 0; i < var_hash->used_slots; i++) {
			zval_ptr_dtor(&var_hash->data[i]);
		}
		next = var_hash->next;
		efree(var_hash);
		var_hash = next;
	}
}