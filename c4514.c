no_context(struct pt_regs *regs, unsigned long error_code,
	   unsigned long address, int signal, int si_code)
{
	struct task_struct *tsk = current;
	unsigned long flags;
	int sig;

	/* Are we prepared to handle this kernel fault? */
	if (fixup_exception(regs)) {
		/*
		 * Any interrupt that takes a fault gets the fixup. This makes
		 * the below recursive fault logic only apply to a faults from
		 * task context.
		 */
		if (in_interrupt())
			return;

		/*
		 * Per the above we're !in_interrupt(), aka. task context.
		 *
		 * In this case we need to make sure we're not recursively
		 * faulting through the emulate_vsyscall() logic.
		 */
		if (current_thread_info()->sig_on_uaccess_error && signal) {
			tsk->thread.trap_nr = X86_TRAP_PF;
			tsk->thread.error_code = error_code | PF_USER;
			tsk->thread.cr2 = address;

			/* XXX: hwpoison faults will set the wrong code. */
			force_sig_info_fault(signal, si_code, address, tsk, 0);
		}

		/*
		 * Barring that, we can do the fixup and be happy.
		 */
		return;
	}

	/*
	 * 32-bit:
	 *
	 *   Valid to do another page fault here, because if this fault
	 *   had been triggered by is_prefetch fixup_exception would have
	 *   handled it.
	 *
	 * 64-bit:
	 *
	 *   Hall of shame of CPU/BIOS bugs.
	 */
	if (is_prefetch(regs, error_code, address))
		return;

	if (is_errata93(regs, address))
		return;

	/*
	 * Oops. The kernel tried to access some bad page. We'll have to
	 * terminate things with extreme prejudice:
	 */
	flags = oops_begin();

	show_fault_oops(regs, error_code, address);

	if (task_stack_end_corrupted(tsk))
		printk(KERN_EMERG "Thread overran stack, or stack corrupted\n");

	tsk->thread.cr2		= address;
	tsk->thread.trap_nr	= X86_TRAP_PF;
	tsk->thread.error_code	= error_code;

	sig = SIGKILL;
	if (__die("Oops", regs, error_code))
		sig = 0;

	/* Executive summary in case the body of the oops scrolled away */
	printk(KERN_DEFAULT "CR2: %016lx\n", address);

	oops_end(flags, regs, sig);
}