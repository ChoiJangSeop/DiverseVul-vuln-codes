void do_notify_resume(struct pt_regs *regs, sigset_t *oldset,
		      __u32 thread_info_flags)
{
	/* Pending single-step? */
	if (thread_info_flags & _TIF_SINGLESTEP)
		clear_thread_flag(TIF_SINGLESTEP);

	/* deal with pending signal delivery */
	if (thread_info_flags & _TIF_SIGPENDING)
		do_signal(regs,oldset);

	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}

	clear_thread_flag(TIF_IRET);
}