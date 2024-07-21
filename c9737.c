void hfi1_mmu_rb_evict(struct mmu_rb_handler *handler, void *evict_arg)
{
	struct mmu_rb_node *rbnode, *ptr;
	struct list_head del_list;
	unsigned long flags;
	bool stop = false;

	INIT_LIST_HEAD(&del_list);

	spin_lock_irqsave(&handler->lock, flags);
	list_for_each_entry_safe_reverse(rbnode, ptr, &handler->lru_list,
					 list) {
		if (handler->ops->evict(handler->ops_arg, rbnode, evict_arg,
					&stop)) {
			__mmu_int_rb_remove(rbnode, &handler->root);
			/* move from LRU list to delete list */
			list_move(&rbnode->list, &del_list);
		}
		if (stop)
			break;
	}
	spin_unlock_irqrestore(&handler->lock, flags);

	while (!list_empty(&del_list)) {
		rbnode = list_first_entry(&del_list, struct mmu_rb_node, list);
		list_del(&rbnode->list);
		handler->ops->remove(handler->ops_arg, rbnode);
	}
}