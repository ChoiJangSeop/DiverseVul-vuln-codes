int ia32_setup_rt_frame(int sig, struct k_sigaction *ka, siginfo_t *info,
			compat_sigset_t *set, struct pt_regs *regs)
{
	struct rt_sigframe __user *frame;
	struct exec_domain *ed = current_thread_info()->exec_domain;
	void __user *restorer;
	int err = 0;

	/* __copy_to_user optimizes that into a single 8 byte store */
	static const struct {
		u8 movl;
		u32 val;
		u16 int80;
		u16 pad;
		u8  pad2;
	} __attribute__((packed)) code = {
		0xb8,
		__NR_ia32_rt_sigreturn,
		0x80cd,
		0,
	};

	frame = get_sigframe(ka, regs, sizeof(*frame));

	if (!access_ok(VERIFY_WRITE, frame, sizeof(*frame)))
		goto give_sigsegv;

	err |= __put_user((ed && ed->signal_invmap && sig < 32
			   ? ed->signal_invmap[sig] : sig), &frame->sig);
	err |= __put_user(ptr_to_compat(&frame->info), &frame->pinfo);
	err |= __put_user(ptr_to_compat(&frame->uc), &frame->puc);
	err |= copy_siginfo_to_user32(&frame->info, info);
	if (err)
		goto give_sigsegv;

	/* Create the ucontext.  */
	err |= __put_user(0, &frame->uc.uc_flags);
	err |= __put_user(0, &frame->uc.uc_link);
	err |= __put_user(current->sas_ss_sp, &frame->uc.uc_stack.ss_sp);
	err |= __put_user(sas_ss_flags(regs->sp),
			  &frame->uc.uc_stack.ss_flags);
	err |= __put_user(current->sas_ss_size, &frame->uc.uc_stack.ss_size);
	err |= ia32_setup_sigcontext(&frame->uc.uc_mcontext, &frame->fpstate,
				     regs, set->sig[0]);
	err |= __copy_to_user(&frame->uc.uc_sigmask, set, sizeof(*set));
	if (err)
		goto give_sigsegv;

	if (ka->sa.sa_flags & SA_RESTORER)
		restorer = ka->sa.sa_restorer;
	else
		restorer = VDSO32_SYMBOL(current->mm->context.vdso,
					 rt_sigreturn);
	err |= __put_user(ptr_to_compat(restorer), &frame->pretcode);

	/*
	 * Not actually used anymore, but left because some gdb
	 * versions need it.
	 */
	err |= __copy_to_user(frame->retcode, &code, 8);
	if (err)
		goto give_sigsegv;

	/* Set up registers for signal handler */
	regs->sp = (unsigned long) frame;
	regs->ip = (unsigned long) ka->sa.sa_handler;

	/* Make -mregparm=3 work */
	regs->ax = sig;
	regs->dx = (unsigned long) &frame->info;
	regs->cx = (unsigned long) &frame->uc;

	/* Make -mregparm=3 work */
	regs->ax = sig;
	regs->dx = (unsigned long) &frame->info;
	regs->cx = (unsigned long) &frame->uc;

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