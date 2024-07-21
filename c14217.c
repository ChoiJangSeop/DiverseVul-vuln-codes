int mempool_destroy(MemoryPoolHandle *handle) {
  struct mempool *pool = NULL;
  struct memory_pool_element *pool_item;
  char *log_msg_fmt = "mempool(%p): free(%p) called for buffer size(%zu)";
  char log_msg[200];

  if (handle == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  pool = (struct mempool *)*handle;
  if (pool == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_lock(&pool->lock);
  }

  if (*handle == NULL) {
    return S3_MEMPOOL_INVALID_ARG;
  }

  /* reset the handle */
  *handle = NULL;
  /* Free the items in free list */
  pool_item = pool->free_list;
  while (pool_item != NULL) {
    pool->free_list = pool_item->next;
    /* Log message about free()'ed item */
    if (pool->log_callback_func) {
      snprintf(log_msg, sizeof(log_msg), log_msg_fmt, (void *)pool,
               (void *)pool_item, pool->mempool_item_size);
      pool->log_callback_func(MEMPOOL_LOG_DEBUG, log_msg);
    }
    free(pool_item);
#if 0
    /* Need this if below asserts are there */
    pool->total_bufs_allocated_by_pool--;
    pool->free_bufs_in_pool--;
#endif
    pool_item = pool->free_list;
  }
  pool->free_list = NULL;

  /* TODO: libevhtp/libevent seems to hold some references and not release back
   * to pool. Bug will be logged for this to investigate.
   */
  /* Assert if there are leaks */
  /*
    assert(pool->total_bufs_allocated_by_pool == 0);
    assert(pool->number_of_bufs_shared == 0);
    assert(pool->free_bufs_in_pool == 0);
  */

  if ((pool->flags & ENABLE_LOCKING) != 0) {
    pthread_mutex_unlock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);
  }

  free(pool);
  pool = NULL;
  return 0;
}