static ssize_t ib_ucm_write(struct file *filp, const char __user *buf,
			    size_t len, loff_t *pos)
{
	struct ib_ucm_file *file = filp->private_data;
	struct ib_ucm_cmd_hdr hdr;
	ssize_t result;

	if (len < sizeof(hdr))
		return -EINVAL;

	if (copy_from_user(&hdr, buf, sizeof(hdr)))
		return -EFAULT;

	if (hdr.cmd >= ARRAY_SIZE(ucm_cmd_table))
		return -EINVAL;

	if (hdr.in + sizeof(hdr) > len)
		return -EINVAL;

	result = ucm_cmd_table[hdr.cmd](file, buf + sizeof(hdr),
					hdr.in, hdr.out);
	if (!result)
		result = len;

	return result;
}