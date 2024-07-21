bool hfi1_mmu_rb_remove_unless_exact(struct mmu_rb_handler *handler,
				     unsigned long addr, unsigned long len,
				     struct mmu_rb_node **rb_node)
{
	struct mmu_rb_node *node;
	unsigned long flags;
	bool ret = false;

	spin_lock_irqsave(&handler->lock, flags);
	node = __mmu_rb_search(handler, addr, len);
	if (node) {
		if (node->addr == addr && node->len == len)
			goto unlock;
		__mmu_int_rb_remove(node, &handler->root);
		list_del(&node->list); /* remove from LRU list */
		ret = true;
	}
unlock:
	spin_unlock_irqrestore(&handler->lock, flags);
	*rb_node = node;
	return ret;
}