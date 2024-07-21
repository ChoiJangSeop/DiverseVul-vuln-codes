do_notify_resume(struct pt_regs *regs, struct switch_stack *sw,
		 unsigned long thread_info_flags,
		 unsigned long r0, unsigned long r19)
{
	if (thread_info_flags & (_TIF_SIGPENDING | _TIF_RESTORE_SIGMASK))
		do_signal(regs, sw, r0, r19);

	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}
}