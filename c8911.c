mptctl_eventenable (unsigned long arg)
{
	struct mpt_ioctl_eventenable __user *uarg = (void __user *) arg;
	struct mpt_ioctl_eventenable	 karg;
	MPT_ADAPTER *ioc;
	int iocnum;

	if (copy_from_user(&karg, uarg, sizeof(struct mpt_ioctl_eventenable))) {
		printk(KERN_ERR MYNAM "%s@%d::mptctl_eventenable - "
			"Unable to read in mpt_ioctl_eventenable struct @ %p\n",
				__FILE__, __LINE__, uarg);
		return -EFAULT;
	}

	if (((iocnum = mpt_verify_adapter(karg.hdr.iocnum, &ioc)) < 0) ||
	    (ioc == NULL)) {
		printk(KERN_DEBUG MYNAM "%s::mptctl_eventenable() @%d - ioc%d not found!\n",
				__FILE__, __LINE__, iocnum);
		return -ENODEV;
	}

	dctlprintk(ioc, printk(MYIOC_s_DEBUG_FMT "mptctl_eventenable called.\n",
	    ioc->name));
	if (ioc->events == NULL) {
		/* Have not yet allocated memory - do so now.
		 */
		int sz = MPTCTL_EVENT_LOG_SIZE * sizeof(MPT_IOCTL_EVENTS);
		ioc->events = kzalloc(sz, GFP_KERNEL);
		if (!ioc->events) {
			printk(MYIOC_s_ERR_FMT
			    ": ERROR - Insufficient memory to add adapter!\n",
			    ioc->name);
			return -ENOMEM;
		}
		ioc->alloc_total += sz;

		ioc->eventContext = 0;
        }

	/* Update the IOC event logging flag.
	 */
	ioc->eventTypes = karg.eventTypes;

	return 0;
}