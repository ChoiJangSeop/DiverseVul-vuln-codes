void hfi1_mmu_rb_unregister(struct mmu_rb_handler *handler)
{
	struct mmu_rb_node *rbnode;
	struct rb_node *node;
	unsigned long flags;
	struct list_head del_list;

	/* Unregister first so we don't get any more notifications. */
	mmu_notifier_unregister(&handler->mn, handler->mm);

	/*
	 * Make sure the wq delete handler is finished running.  It will not
	 * be triggered once the mmu notifiers are unregistered above.
	 */
	flush_work(&handler->del_work);

	INIT_LIST_HEAD(&del_list);

	spin_lock_irqsave(&handler->lock, flags);
	while ((node = rb_first_cached(&handler->root))) {
		rbnode = rb_entry(node, struct mmu_rb_node, node);
		rb_erase_cached(node, &handler->root);
		/* move from LRU list to delete list */
		list_move(&rbnode->list, &del_list);
	}
	spin_unlock_irqrestore(&handler->lock, flags);

	do_remove(handler, &del_list);

	kfree(handler);
}