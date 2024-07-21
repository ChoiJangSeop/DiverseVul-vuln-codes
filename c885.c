asmlinkage void do_notify_resume(struct pt_regs *regs, unsigned int save_r0,
				 unsigned long thread_info_flags)
{
	/* deal with pending signal delivery */
	if (thread_info_flags & _TIF_SIGPENDING)
		do_signal(regs, save_r0);

	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}
}