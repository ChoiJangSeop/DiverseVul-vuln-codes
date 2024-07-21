SYSCALL_DEFINE1(rtas, struct rtas_args __user *, uargs)
{
	struct rtas_args args;
	unsigned long flags;
	char *buff_copy, *errbuf = NULL;
	int nargs, nret, token;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (!rtas.entry)
		return -EINVAL;

	if (copy_from_user(&args, uargs, 3 * sizeof(u32)) != 0)
		return -EFAULT;

	nargs = be32_to_cpu(args.nargs);
	nret  = be32_to_cpu(args.nret);
	token = be32_to_cpu(args.token);

	if (nargs >= ARRAY_SIZE(args.args)
	    || nret > ARRAY_SIZE(args.args)
	    || nargs + nret > ARRAY_SIZE(args.args))
		return -EINVAL;

	/* Copy in args. */
	if (copy_from_user(args.args, uargs->args,
			   nargs * sizeof(rtas_arg_t)) != 0)
		return -EFAULT;

	if (token == RTAS_UNKNOWN_SERVICE)
		return -EINVAL;

	args.rets = &args.args[nargs];
	memset(args.rets, 0, nret * sizeof(rtas_arg_t));

	/* Need to handle ibm,suspend_me call specially */
	if (token == ibm_suspend_me_token) {

		/*
		 * rtas_ibm_suspend_me assumes the streamid handle is in cpu
		 * endian, or at least the hcall within it requires it.
		 */
		int rc = 0;
		u64 handle = ((u64)be32_to_cpu(args.args[0]) << 32)
		              | be32_to_cpu(args.args[1]);
		rc = rtas_ibm_suspend_me(handle);
		if (rc == -EAGAIN)
			args.rets[0] = cpu_to_be32(RTAS_NOT_SUSPENDABLE);
		else if (rc == -EIO)
			args.rets[0] = cpu_to_be32(-1);
		else if (rc)
			return rc;
		goto copy_return;
	}

	buff_copy = get_errorlog_buffer();

	flags = lock_rtas();

	rtas.args = args;
	enter_rtas(__pa(&rtas.args));
	args = rtas.args;

	/* A -1 return code indicates that the last command couldn't
	   be completed due to a hardware error. */
	if (be32_to_cpu(args.rets[0]) == -1)
		errbuf = __fetch_rtas_last_error(buff_copy);

	unlock_rtas(flags);

	if (buff_copy) {
		if (errbuf)
			log_error(errbuf, ERR_TYPE_RTAS_LOG, 0);
		kfree(buff_copy);
	}

 copy_return:
	/* Copy out args. */
	if (copy_to_user(uargs->args + nargs,
			 args.args + nargs,
			 nret * sizeof(rtas_arg_t)) != 0)
		return -EFAULT;

	return 0;
}