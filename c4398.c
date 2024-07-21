void *jas_realloc(void *ptr, size_t size)
{
	void *result;
	jas_mb_t *mb;
	jas_mb_t *old_mb;
	size_t old_size;
	size_t ext_size;
	size_t mem;

	JAS_DBGLOG(100, ("jas_realloc(%x, %zu)\n", ptr, size));
	if (!ptr) {
		return jas_malloc(size);
	}
	if (ptr && !size) {
		jas_free(ptr);
	}
	if (!jas_safe_size_add(size, JAS_MB_SIZE, &ext_size)) {
		jas_eprintf("requested memory size is too large\n");
		return 0;
	}
	old_mb = jas_get_mb(ptr);
	old_size = old_mb->size;
	JAS_DBGLOG(101, ("jas_realloc: old_mb=%x; old_size=%zu\n", old_mb,
	  old_size));
	if (size > old_size) {
		if (!jas_safe_size_add(jas_mem, ext_size, &mem) || mem > jas_max_mem) {
			jas_eprintf("maximum memory limit would be exceeded\n");
			return 0;
		}
	} else {
		if (!jas_safe_size_sub(jas_mem, old_size - size, &jas_mem)) {
			jas_eprintf("heap corruption detected\n");
			abort();
		}
	}
	JAS_DBGLOG(100, ("jas_realloc: realloc(%p, %zu)\n", old_mb, ext_size));
	if (!(mb = realloc(old_mb, ext_size))) {
		result = 0;
	} else {
		result = jas_mb_get_data(mb);
		mb->size = size;
		jas_mem = mem;
	}
	JAS_DBGLOG(100, ("jas_realloc(%p, %zu) -> %p (%p)\n", ptr, size, result,
	  mb));
	JAS_DBGLOG(102, ("max_mem=%zu; mem=%zu\n", jas_max_mem, jas_mem));
	return result;
}