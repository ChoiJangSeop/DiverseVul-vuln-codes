bool timerqueue_del(struct timerqueue_head *head, struct timerqueue_node *node)
{
	WARN_ON_ONCE(RB_EMPTY_NODE(&node->node));

	/* update next pointer */
	if (head->next == node) {
		struct rb_node *rbn = rb_next(&node->node);

		head->next = rb_entry_safe(rbn, struct timerqueue_node, node);
	}
	rb_erase(&node->node, &head->head);
	RB_CLEAR_NODE(&node->node);
	return head->next != NULL;
}