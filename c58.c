static int setup_rt_frame(int sig, struct k_sigaction *ka, siginfo_t *info,
			   sigset_t *set, struct pt_regs * regs)
{
	void __user *restorer;
	struct rt_sigframe __user *frame;
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

	err |= __put_user(usig, &frame->sig);
	err |= __put_user(&frame->info, &frame->pinfo);
	err |= __put_user(&frame->uc, &frame->puc);
	err |= copy_siginfo_to_user(&frame->info, info);
	if (err)
		goto give_sigsegv;

	/* Create the ucontext.  */
	err |= __put_user(0, &frame->uc.uc_flags);
	err |= __put_user(0, &frame->uc.uc_link);
	err |= __put_user(current->sas_ss_sp, &frame->uc.uc_stack.ss_sp);
	err |= __put_user(sas_ss_flags(regs->sp),
			  &frame->uc.uc_stack.ss_flags);
	err |= __put_user(current->sas_ss_size, &frame->uc.uc_stack.ss_size);
	err |= setup_sigcontext(&frame->uc.uc_mcontext, &frame->fpstate,
			        regs, set->sig[0]);
	err |= __copy_to_user(&frame->uc.uc_sigmask, set, sizeof(*set));
	if (err)
		goto give_sigsegv;

	/* Set up to return from userspace.  */
	restorer = VDSO32_SYMBOL(current->mm->context.vdso, rt_sigreturn);
	if (ka->sa.sa_flags & SA_RESTORER)
		restorer = ka->sa.sa_restorer;
	err |= __put_user(restorer, &frame->pretcode);
	 
	/*
	 * This is movl $,%ax ; int $0x80
	 *
	 * WE DO NOT USE IT ANY MORE! It's only left here for historical
	 * reasons and because gdb uses it as a signature to notice
	 * signal handler stack frames.
	 */
	err |= __put_user(0xb8, (char __user *)(frame->retcode+0));
	err |= __put_user(__NR_rt_sigreturn, (int __user *)(frame->retcode+1));
	err |= __put_user(0x80cd, (short __user *)(frame->retcode+5));

	if (err)
		goto give_sigsegv;

	/* Set up registers for signal handler */
	regs->sp = (unsigned long) frame;
	regs->ip = (unsigned long) ka->sa.sa_handler;
	regs->ax = (unsigned long) usig;
	regs->dx = (unsigned long) &frame->info;
	regs->cx = (unsigned long) &frame->uc;

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