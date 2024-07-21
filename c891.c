asmlinkage void do_notify_resume(struct pt_regs *regs, struct thread_info *ti)
{
	int syscall = 0;

	if ((sysreg_read(SR) & MODE_MASK) == MODE_SUPERVISOR)
		syscall = 1;

	if (ti->flags & (_TIF_SIGPENDING | _TIF_RESTORE_SIGMASK))
		do_signal(regs, &current->blocked, syscall);

	if (ti->flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}
}