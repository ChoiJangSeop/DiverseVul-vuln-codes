ZEND_API void zend_ts_hash_graceful_destroy(TsHashTable *ht)
{
	begin_write(ht);
	zend_hash_graceful_destroy(TS_HASH(ht));
	end_write(ht);

#ifdef ZTS
	tsrm_mutex_free(ht->mx_reader);
	tsrm_mutex_free(ht->mx_reader);
#endif
}