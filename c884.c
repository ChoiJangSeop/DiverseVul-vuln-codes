void do_notify_resume(struct pt_regs *regs, long in_syscall)
{
	if (test_thread_flag(TIF_SIGPENDING) ||
	    test_thread_flag(TIF_RESTORE_SIGMASK))
		do_signal(regs, in_syscall);

	if (test_thread_flag(TIF_NOTIFY_RESUME)) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}
}