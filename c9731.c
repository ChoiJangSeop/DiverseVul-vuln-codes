int hfi1_mmu_rb_insert(struct mmu_rb_handler *handler,
		       struct mmu_rb_node *mnode)
{
	struct mmu_rb_node *node;
	unsigned long flags;
	int ret = 0;

	trace_hfi1_mmu_rb_insert(mnode->addr, mnode->len);
	spin_lock_irqsave(&handler->lock, flags);
	node = __mmu_rb_search(handler, mnode->addr, mnode->len);
	if (node) {
		ret = -EINVAL;
		goto unlock;
	}
	__mmu_int_rb_insert(mnode, &handler->root);
	list_add(&mnode->list, &handler->lru_list);

	ret = handler->ops->insert(handler->ops_arg, mnode);
	if (ret) {
		__mmu_int_rb_remove(mnode, &handler->root);
		list_del(&mnode->list); /* remove from LRU list */
	}
unlock:
	spin_unlock_irqrestore(&handler->lock, flags);
	return ret;
}