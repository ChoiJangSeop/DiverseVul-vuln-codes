void *_zend_shared_memdup(void *source, size_t size, zend_bool free_source)
{
	void *old_p, *retval;

	if ((old_p = zend_hash_index_find_ptr(&xlat_table, (zend_ulong)source)) != NULL) {
		/* we already duplicated this pointer */
		return old_p;
	}
	retval = ZCG(mem);
	ZCG(mem) = (void*)(((char*)ZCG(mem)) + ZEND_ALIGNED_SIZE(size));
	memcpy(retval, source, size);
	if (free_source) {
		efree(source);
	}
	zend_shared_alloc_register_xlat_entry(source, retval);
	return retval;
}