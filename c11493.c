static bool io_match_task_safe(struct io_kiocb *head, struct task_struct *task,
			       bool cancel_all)
{
	if (task && head->task != task)
		return false;
	return cancel_all;
}