mptctl_mpt_command (unsigned long arg)
{
	struct mpt_ioctl_command __user *uarg = (void __user *) arg;
	struct mpt_ioctl_command  karg;
	MPT_ADAPTER	*ioc;
	int		iocnum;
	int		rc;


	if (copy_from_user(&karg, uarg, sizeof(struct mpt_ioctl_command))) {
		printk(KERN_ERR MYNAM "%s@%d::mptctl_mpt_command - "
			"Unable to read in mpt_ioctl_command struct @ %p\n",
				__FILE__, __LINE__, uarg);
		return -EFAULT;
	}

	if (((iocnum = mpt_verify_adapter(karg.hdr.iocnum, &ioc)) < 0) ||
	    (ioc == NULL)) {
		printk(KERN_DEBUG MYNAM "%s::mptctl_mpt_command() @%d - ioc%d not found!\n",
				__FILE__, __LINE__, iocnum);
		return -ENODEV;
	}

	rc = mptctl_do_mpt_command (karg, &uarg->MF);

	return rc;
}