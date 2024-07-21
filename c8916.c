mptctl_replace_fw (unsigned long arg)
{
	struct mpt_ioctl_replace_fw __user *uarg = (void __user *) arg;
	struct mpt_ioctl_replace_fw	 karg;
	MPT_ADAPTER		 *ioc;
	int			 iocnum;
	int			 newFwSize;

	if (copy_from_user(&karg, uarg, sizeof(struct mpt_ioctl_replace_fw))) {
		printk(KERN_ERR MYNAM "%s@%d::mptctl_replace_fw - "
			"Unable to read in mpt_ioctl_replace_fw struct @ %p\n",
				__FILE__, __LINE__, uarg);
		return -EFAULT;
	}

	if (((iocnum = mpt_verify_adapter(karg.hdr.iocnum, &ioc)) < 0) ||
	    (ioc == NULL)) {
		printk(KERN_DEBUG MYNAM "%s::mptctl_replace_fw() @%d - ioc%d not found!\n",
				__FILE__, __LINE__, iocnum);
		return -ENODEV;
	}

	dctlprintk(ioc, printk(MYIOC_s_DEBUG_FMT "mptctl_replace_fw called.\n",
	    ioc->name));
	/* If caching FW, Free the old FW image
	 */
	if (ioc->cached_fw == NULL)
		return 0;

	mpt_free_fw_memory(ioc);

	/* Allocate memory for the new FW image
	 */
	newFwSize = ALIGN(karg.newImageSize, 4);

	mpt_alloc_fw_memory(ioc, newFwSize);
	if (ioc->cached_fw == NULL)
		return -ENOMEM;

	/* Copy the data from user memory to kernel space
	 */
	if (copy_from_user(ioc->cached_fw, uarg->newImage, newFwSize)) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_replace_fw - "
				"Unable to read in mpt_ioctl_replace_fw image "
				"@ %p\n", ioc->name, __FILE__, __LINE__, uarg);
		mpt_free_fw_memory(ioc);
		return -EFAULT;
	}

	/* Update IOCFactsReply
	 */
	ioc->facts.FWImageSize = newFwSize;
	return 0;
}