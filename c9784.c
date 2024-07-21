static int bprm_execve(struct linux_binprm *bprm,
		       int fd, struct filename *filename, int flags)
{
	struct file *file;
	struct files_struct *displaced;
	int retval;

	retval = unshare_files(&displaced);
	if (retval)
		return retval;

	retval = prepare_bprm_creds(bprm);
	if (retval)
		goto out_files;

	check_unsafe_exec(bprm);
	current->in_execve = 1;

	file = do_open_execat(fd, filename, flags);
	retval = PTR_ERR(file);
	if (IS_ERR(file))
		goto out_unmark;

	sched_exec();

	bprm->file = file;
	/*
	 * Record that a name derived from an O_CLOEXEC fd will be
	 * inaccessible after exec. Relies on having exclusive access to
	 * current->files (due to unshare_files above).
	 */
	if (bprm->fdpath &&
	    close_on_exec(fd, rcu_dereference_raw(current->files->fdt)))
		bprm->interp_flags |= BINPRM_FLAGS_PATH_INACCESSIBLE;

	/* Set the unchanging part of bprm->cred */
	retval = security_bprm_creds_for_exec(bprm);
	if (retval)
		goto out;

	retval = exec_binprm(bprm);
	if (retval < 0)
		goto out;

	/* execve succeeded */
	current->fs->in_exec = 0;
	current->in_execve = 0;
	rseq_execve(current);
	acct_update_integrals(current);
	task_numa_free(current, false);
	if (displaced)
		put_files_struct(displaced);
	return retval;

out:
	/*
	 * If past the point of no return ensure the the code never
	 * returns to the userspace process.  Use an existing fatal
	 * signal if present otherwise terminate the process with
	 * SIGSEGV.
	 */
	if (bprm->point_of_no_return && !fatal_signal_pending(current))
		force_sigsegv(SIGSEGV);

out_unmark:
	current->fs->in_exec = 0;
	current->in_execve = 0;

out_files:
	if (displaced)
		reset_files_struct(displaced);

	return retval;
}