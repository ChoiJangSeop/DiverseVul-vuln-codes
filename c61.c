static int setup_frame(int sig, struct k_sigaction *ka,
		       sigset_t *set, struct pt_regs * regs)
{
	void __user *restorer;
	struct sigframe __user *frame;
	int err = 0;
	int usig;

	frame = get_sigframe(ka, regs, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		goto give_sigsegv;

	usig = current_thread_info()->exec_domain
		&& current_thread_info()->exec_domain->signal_invmap
		&& sig < 32
		? current_thread_info()->exec_domain->signal_invmap[sig]
		: sig;

	err = __put_user(usig, &frame->sig);
	if (err)
		goto give_sigsegv;

	err = setup_sigcontext(&frame->sc, &frame->fpstate, regs, set->sig[0]);
	if (err)
		goto give_sigsegv;

	if (_NSIG_WORDS > 1) {
		err = __copy_to_user(&frame->extramask, &set->sig[1],
				      sizeof(frame->extramask));
		if (err)
			goto give_sigsegv;
	}

	if (current->binfmt->hasvdso)
		restorer = VDSO32_SYMBOL(current->mm->context.vdso, sigreturn);
	else
		restorer = &frame->retcode;
	if (ka->sa.sa_flags & SA_RESTORER)
		restorer = ka->sa.sa_restorer;

	/* Set up to return from userspace.  */
	err |= __put_user(restorer, &frame->pretcode);
	 
	/*
	 * This is popl %eax ; movl $,%eax ; int $0x80
	 *
	 * WE DO NOT USE IT ANY MORE! It's only left here for historical
	 * reasons and because gdb uses it as a signature to notice
	 * signal handler stack frames.
	 */
	err |= __put_user(0xb858, (short __user *)(frame->retcode+0));
	err |= __put_user(__NR_sigreturn, (int __user *)(frame->retcode+2));
	err |= __put_user(0x80cd, (short __user *)(frame->retcode+6));

	if (err)
		goto give_sigsegv;

	/* Set up registers for signal handler */
	regs->sp = (unsigned long) frame;
	regs->ip = (unsigned long) ka->sa.sa_handler;
	regs->ax = (unsigned long) sig;
	regs->dx = (unsigned long) 0;
	regs->cx = (unsigned long) 0;

	regs->ds = __USER_DS;
	regs->es = __USER_DS;
	regs->ss = __USER_DS;
	regs->cs = __USER_CS;

	/*
	 * Clear TF when entering the signal handler, but
	 * notify any tracer that was single-stepping it.
	 * The tracer may want to single-step inside the
	 * handler too.
	 */
	regs->flags &= ~TF_MASK;
	if (test_thread_flag(TIF_SINGLESTEP))
		ptrace_notify(SIGTRAP);

#if DEBUG_SIG
	printk("SIG deliver (%s:%d): sp=%p pc=%p ra=%p\n",
		current->comm, current->pid, frame, regs->ip, frame->pretcode);
#endif

	return 0;

give_sigsegv:
	force_sigsegv(sig, current);
	return -EFAULT;
}