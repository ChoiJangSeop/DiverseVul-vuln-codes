PHPAPI void var_replace(php_unserialize_data_t *var_hashx, zval *ozval, zval **nzval)
{
	long i;
	var_entries *var_hash = (*var_hashx)->first;
#if VAR_ENTRIES_DBG
	fprintf(stderr, "var_replace(%ld): %d\n", var_hash?var_hash->used_slots:-1L, Z_TYPE_PP(nzval));
#endif
	
	while (var_hash) {
		for (i = 0; i < var_hash->used_slots; i++) {
			if (var_hash->data[i] == ozval) {
				var_hash->data[i] = *nzval;
				/* do not break here */
			}
		}
		var_hash = var_hash->next;
	}
}