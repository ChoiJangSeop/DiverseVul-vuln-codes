do_notify_resume(struct pt_regs *regs, void *unused, __u32 thread_info_flags)
{
#ifdef CONFIG_X86_MCE
	/* notify userspace of pending MCEs */
	if (thread_info_flags & _TIF_MCE_NOTIFY)
		mce_notify_process();
#endif /* CONFIG_X86_64 && CONFIG_X86_MCE */

	/* deal with pending signal delivery */
	if (thread_info_flags & _TIF_SIGPENDING)
		do_signal(regs);

	if (thread_info_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
		if (current->replacement_session_keyring)
			key_replace_session_keyring();
	}
	if (thread_info_flags & _TIF_USER_RETURN_NOTIFY)
		fire_user_return_notifiers();

#ifdef CONFIG_X86_32
	clear_thread_flag(TIF_IRET);
#endif /* CONFIG_X86_32 */
}