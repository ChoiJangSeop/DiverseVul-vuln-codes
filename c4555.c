static int get_task_ioprio(struct task_struct *p)
{
	int ret;

	ret = security_task_getioprio(p);
	if (ret)
		goto out;
	ret = IOPRIO_PRIO_VALUE(IOPRIO_CLASS_NONE, IOPRIO_NORM);
	if (p->io_context)
		ret = p->io_context->ioprio;
out:
	return ret;
}