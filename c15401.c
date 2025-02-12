void exit_io_context(void)
{
	struct io_context *ioc;

	task_lock(current);
	ioc = current->io_context;
	current->io_context = NULL;
	task_unlock(current);

	if (atomic_dec_and_test(&ioc->nr_tasks)) {
		if (ioc->aic && ioc->aic->exit)
			ioc->aic->exit(ioc->aic);
		cfq_exit(ioc);

		put_io_context(ioc);
	}
}