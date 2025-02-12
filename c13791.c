static int ptrace_check_attach(struct task_struct *child, bool ignore_state)
{
	int ret = -ESRCH;

	/*
	 * We take the read lock around doing both checks to close a
	 * possible race where someone else was tracing our child and
	 * detached between these two checks.  After this locked check,
	 * we are sure that this is our traced child and that can only
	 * be changed by us so it's not changing right after this.
	 */
	read_lock(&tasklist_lock);
	if ((child->ptrace & PT_PTRACED) && child->parent == current) {
		/*
		 * child->sighand can't be NULL, release_task()
		 * does ptrace_unlink() before __exit_signal().
		 */
		spin_lock_irq(&child->sighand->siglock);
		WARN_ON_ONCE(task_is_stopped(child));
		if (ignore_state || (task_is_traced(child) &&
				     !(child->jobctl & JOBCTL_LISTENING)))
			ret = 0;
		spin_unlock_irq(&child->sighand->siglock);
	}
	read_unlock(&tasklist_lock);

	if (!ret && !ignore_state)
		ret = wait_task_inactive(child, TASK_TRACED) ? 0 : -ESRCH;

	/* All systems go.. */
	return ret;
}