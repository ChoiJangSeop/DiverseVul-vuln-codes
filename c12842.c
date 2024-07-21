void *umm_calloc( size_t num, size_t item_size ) {
  void *ret;
  size_t size = item_size * num;

  /* check poison of each blocks, if poisoning is enabled */
  if (!CHECK_POISON_ALL_BLOCKS()) {
    return NULL;
  }

  /* check full integrity of the heap, if this check is enabled */
  if (!INTEGRITY_CHECK()) {
    return NULL;
  }

  size += POISON_SIZE(size);
  ret = _umm_malloc(size);
  if (ret != NULL) memset(ret, 0x00, size);

  ret = GET_POISONED(ret, size);

  umm_account_free_blocks_cnt();

  return ret;
}