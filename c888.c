asmlinkage void do_notify_resume(__u32 thread_info_flags)
{
	/* pending single-step? */
	if (thread_info_flags & _TIF_SINGLESTEP)
		clear_thread_flag(TIF_SINGLESTEP);

	/* deal with pending signal delivery */
	if (thread_info_flags & (_TIF_SIGPENDING | _TIF_RESTORE_SIGMASK))
		do_signal();

	/* deal with notification on about to resume userspace execution */
	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(__frame);
	}

} /* end do_notify_resume() */