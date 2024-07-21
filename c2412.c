die(SIGNAL_ARGS)
{
	int			save_errno = errno;

	/* Don't joggle the elbow of proc_exit */
	if (!proc_exit_inprogress)
	{
		InterruptPending = true;
		ProcDiePending = true;

		/*
		 * If it's safe to interrupt, and we're waiting for input or a lock,
		 * service the interrupt immediately
		 */
		if (ImmediateInterruptOK && InterruptHoldoffCount == 0 &&
			CritSectionCount == 0)
		{
			/* bump holdoff count to make ProcessInterrupts() a no-op */
			/* until we are done getting ready for it */
			InterruptHoldoffCount++;
			LockErrorCleanup(); /* prevent CheckDeadLock from running */
			DisableNotifyInterrupt();
			DisableCatchupInterrupt();
			InterruptHoldoffCount--;
			ProcessInterrupts();
		}
	}

	/* If we're still here, waken anything waiting on the process latch */
	SetLatch(MyLatch);

	errno = save_errno;
}