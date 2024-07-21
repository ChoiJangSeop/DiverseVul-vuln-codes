int hashbin_delete( hashbin_t* hashbin, FREE_FUNC free_func)
{
	irda_queue_t* queue;
	unsigned long flags = 0;
	int i;

	IRDA_ASSERT(hashbin != NULL, return -1;);
	IRDA_ASSERT(hashbin->magic == HB_MAGIC, return -1;);

	/* Synchronize */
	if ( hashbin->hb_type & HB_LOCK ) {
		spin_lock_irqsave_nested(&hashbin->hb_spinlock, flags,
					 hashbin_lock_depth++);
	}

	/*
	 *  Free the entries in the hashbin, TODO: use hashbin_clear when
	 *  it has been shown to work
	 */
	for (i = 0; i < HASHBIN_SIZE; i ++ ) {
		queue = dequeue_first((irda_queue_t**) &hashbin->hb_queue[i]);
		while (queue ) {
			if (free_func)
				(*free_func)(queue);
			queue = dequeue_first(
				(irda_queue_t**) &hashbin->hb_queue[i]);
		}
	}

	/* Cleanup local data */
	hashbin->hb_current = NULL;
	hashbin->magic = ~HB_MAGIC;

	/* Release lock */
	if ( hashbin->hb_type & HB_LOCK) {
		spin_unlock_irqrestore(&hashbin->hb_spinlock, flags);
#ifdef CONFIG_LOCKDEP
		hashbin_lock_depth--;
#endif
	}

	/*
	 *  Free the hashbin structure
	 */
	kfree(hashbin);

	return 0;
}