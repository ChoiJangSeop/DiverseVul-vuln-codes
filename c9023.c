static ssize_t psi_write(struct file *file, const char __user *user_buf,
			 size_t nbytes, enum psi_res res)
{
	char buf[32];
	size_t buf_size;
	struct seq_file *seq;
	struct psi_trigger *new;

	if (static_branch_likely(&psi_disabled))
		return -EOPNOTSUPP;

	buf_size = min(nbytes, sizeof(buf));
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;

	buf[buf_size - 1] = '\0';

	new = psi_trigger_create(&psi_system, buf, nbytes, res);
	if (IS_ERR(new))
		return PTR_ERR(new);

	seq = file->private_data;
	/* Take seq->lock to protect seq->private from concurrent writes */
	mutex_lock(&seq->lock);
	psi_trigger_replace(&seq->private, new);
	mutex_unlock(&seq->lock);

	return nbytes;
}