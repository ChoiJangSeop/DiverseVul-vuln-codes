static void audit_log_execve_info(struct audit_context *context,
				  struct audit_buffer **ab)
{
	int i, len;
	size_t len_sent = 0;
	const char __user *p;
	char *buf;

	p = (const char __user *)current->mm->arg_start;

	audit_log_format(*ab, "argc=%d", context->execve.argc);

	/*
	 * we need some kernel buffer to hold the userspace args.  Just
	 * allocate one big one rather than allocating one of the right size
	 * for every single argument inside audit_log_single_execve_arg()
	 * should be <8k allocation so should be pretty safe.
	 */
	buf = kmalloc(MAX_EXECVE_AUDIT_LEN + 1, GFP_KERNEL);
	if (!buf) {
		audit_panic("out of memory for argv string");
		return;
	}

	for (i = 0; i < context->execve.argc; i++) {
		len = audit_log_single_execve_arg(context, ab, i,
						  &len_sent, p, buf);
		if (len <= 0)
			break;
		p += len;
	}
	kfree(buf);
}