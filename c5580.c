void tm_recheckpoint(struct thread_struct *thread,
		     unsigned long orig_msr)
{
	unsigned long flags;

	/* We really can't be interrupted here as the TEXASR registers can't
	 * change and later in the trecheckpoint code, we have a userspace R1.
	 * So let's hard disable over this region.
	 */
	local_irq_save(flags);
	hard_irq_disable();

	/* The TM SPRs are restored here, so that TEXASR.FS can be set
	 * before the trecheckpoint and no explosion occurs.
	 */
	tm_restore_sprs(thread);

	__tm_recheckpoint(thread, orig_msr);

	local_irq_restore(flags);
}