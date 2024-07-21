find_lively_task_by_vpid(pid_t vpid)
{
	struct task_struct *task;
	int err;

	rcu_read_lock();
	if (!vpid)
		task = current;
	else
		task = find_task_by_vpid(vpid);
	if (task)
		get_task_struct(task);
	rcu_read_unlock();

	if (!task)
		return ERR_PTR(-ESRCH);

	/* Reuse ptrace permission checks for now. */
	err = -EACCES;
	if (!ptrace_may_access(task, PTRACE_MODE_READ_REALCREDS))
		goto errout;

	return task;
errout:
	put_task_struct(task);
	return ERR_PTR(err);

}