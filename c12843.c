void *umm_realloc( void *ptr, size_t size ) {
  void *ret;

  ptr = GET_UNPOISONED(ptr);

  /* check poison of each blocks, if poisoning is enabled */
  if (!CHECK_POISON_ALL_BLOCKS()) {
    return NULL;
  }

  /* check full integrity of the heap, if this check is enabled */
  if (!INTEGRITY_CHECK()) {
    return NULL;
  }

  size += POISON_SIZE(size);
  ret = _umm_realloc( ptr, size );

  ret = GET_POISONED(ret, size);

  umm_account_free_blocks_cnt();

  return ret;
}