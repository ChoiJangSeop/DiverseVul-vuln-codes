void hfi1_mmu_rb_remove(struct mmu_rb_handler *handler,
			struct mmu_rb_node *node)
{
	unsigned long flags;

	/* Validity of handler and node pointers has been checked by caller. */
	trace_hfi1_mmu_rb_remove(node->addr, node->len);
	spin_lock_irqsave(&handler->lock, flags);
	__mmu_int_rb_remove(node, &handler->root);
	list_del(&node->list); /* remove from LRU list */
	spin_unlock_irqrestore(&handler->lock, flags);

	handler->ops->remove(handler->ops_arg, node);
}