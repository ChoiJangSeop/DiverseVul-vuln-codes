static void tm_unavailable(struct pt_regs *regs)
{
	pr_emerg("Unrecoverable TM Unavailable Exception "
			"%lx at %lx\n", regs->trap, regs->nip);
	die("Unrecoverable TM Unavailable Exception", regs, SIGABRT);
}