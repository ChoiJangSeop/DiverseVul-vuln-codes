asmlinkage void do_notify_resume(struct pt_regs *regs, void *unused,
	__u32 thread_info_flags)
{
	/* deal with pending signal delivery */
	if (thread_info_flags & (_TIF_SIGPENDING | _TIF_RESTORE_SIGMASK))
		do_signal(regs);

	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}
}