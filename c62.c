int ia32_setup_frame(int sig, struct k_sigaction *ka,
		     compat_sigset_t *set, struct pt_regs *regs)
{
	struct sigframe __user *frame;
	void __user *restorer;
	int err = 0;

	/* copy_to_user optimizes that into a single 8 byte store */
	static const struct {
		u16 poplmovl;
		u32 val;
		u16 int80;
		u16 pad;
	} __attribute__((packed)) code = {
		0xb858,		 /* popl %eax ; movl $...,%eax */
		__NR_ia32_sigreturn,
		0x80cd,		/* int $0x80 */
		0,
	};

	frame = get_sigframe(ka, regs, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		goto give_sigsegv;

	err |= __put_user(sig, &frame->sig);
	if (err)
		goto give_sigsegv;

	err |= ia32_setup_sigcontext(&frame->sc, &frame->fpstate, regs,
					set->sig[0]);
	if (err)
		goto give_sigsegv;

	if (_COMPAT_NSIG_WORDS > 1) {
		err |= __copy_to_user(frame->extramask, &set->sig[1],
				      sizeof(frame->extramask));
		if (err)
			goto give_sigsegv;
	}

	if (ka->sa.sa_flags & SA_RESTORER) {
		restorer = ka->sa.sa_restorer;
	} else {
		/* Return stub is in 32bit vsyscall page */
		if (current->binfmt->hasvdso)
			restorer = VDSO32_SYMBOL(current->mm->context.vdso,
						 sigreturn);
		else
			restorer = &frame->retcode;
	}
	err |= __put_user(ptr_to_compat(restorer), &frame->pretcode);

	/*
	 * These are actually not used anymore, but left because some
	 * gdb versions depend on them as a marker.
	 */
	err |= __copy_to_user(frame->retcode, &code, 8);
	if (err)
		goto give_sigsegv;

	/* Set up registers for signal handler */
	regs->sp = (unsigned long) frame;
	regs->ip = (unsigned long) ka->sa.sa_handler;

	/* Make -mregparm=3 work */
	regs->ax = sig;
	regs->dx = 0;
	regs->cx = 0;

	asm volatile("movl %0,%%ds" :: "r" (__USER32_DS));
	asm volatile("movl %0,%%es" :: "r" (__USER32_DS));

	regs->cs = __USER32_CS;
	regs->ss = __USER32_DS;

	set_fs(USER_DS);
	regs->flags &= ~X86_EFLAGS_TF;
	if (test_thread_flag(TIF_SINGLESTEP))
		ptrace_notify(SIGTRAP);

#if DEBUG_SIG
	printk(KERN_DEBUG "SIG deliver (%s:%d): sp=%p pc=%lx ra=%u\n",
	       current->comm, current->pid, frame, regs->ip, frame->pretcode);
#endif

	return 0;

give_sigsegv:
	force_sigsegv(sig, current);
	return -EFAULT;
}