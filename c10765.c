struct timerqueue_node *timerqueue_getnext(struct timerqueue_head *head)
{
	return head->next;
}