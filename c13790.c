	__acquires(&current->sighand->siglock)
{
	bool gstop_done = false;

	if (arch_ptrace_stop_needed(exit_code, info)) {
		/*
		 * The arch code has something special to do before a
		 * ptrace stop.  This is allowed to block, e.g. for faults
		 * on user stack pages.  We can't keep the siglock while
		 * calling arch_ptrace_stop, so we must release it now.
		 * To preserve proper semantics, we must do this before
		 * any signal bookkeeping like checking group_stop_count.
		 * Meanwhile, a SIGKILL could come in before we retake the
		 * siglock.  That must prevent us from sleeping in TASK_TRACED.
		 * So after regaining the lock, we must check for SIGKILL.
		 */
		spin_unlock_irq(&current->sighand->siglock);
		arch_ptrace_stop(exit_code, info);
		spin_lock_irq(&current->sighand->siglock);
		if (sigkill_pending(current))
			return;
	}

	/*
	 * We're committing to trapping.  TRACED should be visible before
	 * TRAPPING is cleared; otherwise, the tracer might fail do_wait().
	 * Also, transition to TRACED and updates to ->jobctl should be
	 * atomic with respect to siglock and should be done after the arch
	 * hook as siglock is released and regrabbed across it.
	 */
	set_current_state(TASK_TRACED);

	current->last_siginfo = info;
	current->exit_code = exit_code;

	/*
	 * If @why is CLD_STOPPED, we're trapping to participate in a group
	 * stop.  Do the bookkeeping.  Note that if SIGCONT was delievered
	 * across siglock relocks since INTERRUPT was scheduled, PENDING
	 * could be clear now.  We act as if SIGCONT is received after
	 * TASK_TRACED is entered - ignore it.
	 */
	if (why == CLD_STOPPED && (current->jobctl & JOBCTL_STOP_PENDING))
		gstop_done = task_participate_group_stop(current);

	/* any trap clears pending STOP trap, STOP trap clears NOTIFY */
	task_clear_jobctl_pending(current, JOBCTL_TRAP_STOP);
	if (info && info->si_code >> 8 == PTRACE_EVENT_STOP)
		task_clear_jobctl_pending(current, JOBCTL_TRAP_NOTIFY);

	/* entering a trap, clear TRAPPING */
	task_clear_jobctl_trapping(current);

	spin_unlock_irq(&current->sighand->siglock);
	read_lock(&tasklist_lock);
	if (may_ptrace_stop()) {
		/*
		 * Notify parents of the stop.
		 *
		 * While ptraced, there are two parents - the ptracer and
		 * the real_parent of the group_leader.  The ptracer should
		 * know about every stop while the real parent is only
		 * interested in the completion of group stop.  The states
		 * for the two don't interact with each other.  Notify
		 * separately unless they're gonna be duplicates.
		 */
		do_notify_parent_cldstop(current, true, why);
		if (gstop_done && ptrace_reparented(current))
			do_notify_parent_cldstop(current, false, why);

		/*
		 * Don't want to allow preemption here, because
		 * sys_ptrace() needs this task to be inactive.
		 *
		 * XXX: implement read_unlock_no_resched().
		 */
		preempt_disable();
		read_unlock(&tasklist_lock);
		preempt_enable_no_resched();
		freezable_schedule();
	} else {
		/*
		 * By the time we got the lock, our tracer went away.
		 * Don't drop the lock yet, another tracer may come.
		 *
		 * If @gstop_done, the ptracer went away between group stop
		 * completion and here.  During detach, it would have set
		 * JOBCTL_STOP_PENDING on us and we'll re-enter
		 * TASK_STOPPED in do_signal_stop() on return, so notifying
		 * the real parent of the group stop completion is enough.
		 */
		if (gstop_done)
			do_notify_parent_cldstop(current, false, why);

		__set_current_state(TASK_RUNNING);
		if (clear_code)
			current->exit_code = 0;
		read_unlock(&tasklist_lock);
	}

	/*
	 * We are back.  Now reacquire the siglock before touching
	 * last_siginfo, so that we are sure to have synchronized with
	 * any signal-sending on another CPU that wants to examine it.
	 */
	spin_lock_irq(&current->sighand->siglock);
	current->last_siginfo = NULL;

	/* LISTENING can be set only during STOP traps, clear it */
	current->jobctl &= ~JOBCTL_LISTENING;

	/*
	 * Queued signals ignored us while we were stopped for tracing.
	 * So check for any that we should take before resuming user mode.
	 * This sets TIF_SIGPENDING, but never clears it.
	 */
	recalc_sigpending_tsk(current);
}