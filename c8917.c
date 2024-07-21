mptctl_eventquery (unsigned long arg)
{
	struct mpt_ioctl_eventquery __user *uarg = (void __user *) arg;
	struct mpt_ioctl_eventquery	 karg;
	MPT_ADAPTER *ioc;
	int iocnum;

	if (copy_from_user(&karg, uarg, sizeof(struct mpt_ioctl_eventquery))) {
		printk(KERN_ERR MYNAM "%s@%d::mptctl_eventquery - "
			"Unable to read in mpt_ioctl_eventquery struct @ %p\n",
				__FILE__, __LINE__, uarg);
		return -EFAULT;
	}

	if (((iocnum = mpt_verify_adapter(karg.hdr.iocnum, &ioc)) < 0) ||
	    (ioc == NULL)) {
		printk(KERN_DEBUG MYNAM "%s::mptctl_eventquery() @%d - ioc%d not found!\n",
				__FILE__, __LINE__, iocnum);
		return -ENODEV;
	}

	dctlprintk(ioc, printk(MYIOC_s_DEBUG_FMT "mptctl_eventquery called.\n",
	    ioc->name));
	karg.eventEntries = MPTCTL_EVENT_LOG_SIZE;
	karg.eventTypes = ioc->eventTypes;

	/* Copy the data from kernel memory to user memory
	 */
	if (copy_to_user((char __user *)arg, &karg, sizeof(struct mpt_ioctl_eventquery))) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_eventquery - "
			"Unable to write out mpt_ioctl_eventquery struct @ %p\n",
			ioc->name, __FILE__, __LINE__, uarg);
		return -EFAULT;
	}
	return 0;
}