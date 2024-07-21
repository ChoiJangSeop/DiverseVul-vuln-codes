mptctl_gettargetinfo (unsigned long arg)
{
	struct mpt_ioctl_targetinfo __user *uarg = (void __user *) arg;
	struct mpt_ioctl_targetinfo karg;
	MPT_ADAPTER		*ioc;
	VirtDevice		*vdevice;
	char			*pmem;
	int			*pdata;
	int			iocnum;
	int			numDevices = 0;
	int			lun;
	int			maxWordsLeft;
	int			numBytes;
	u8			port;
	struct scsi_device 	*sdev;

	if (copy_from_user(&karg, uarg, sizeof(struct mpt_ioctl_targetinfo))) {
		printk(KERN_ERR MYNAM "%s@%d::mptctl_gettargetinfo - "
			"Unable to read in mpt_ioctl_targetinfo struct @ %p\n",
				__FILE__, __LINE__, uarg);
		return -EFAULT;
	}

	if (((iocnum = mpt_verify_adapter(karg.hdr.iocnum, &ioc)) < 0) ||
	    (ioc == NULL)) {
		printk(KERN_DEBUG MYNAM "%s::mptctl_gettargetinfo() @%d - ioc%d not found!\n",
				__FILE__, __LINE__, iocnum);
		return -ENODEV;
	}

	dctlprintk(ioc, printk(MYIOC_s_DEBUG_FMT "mptctl_gettargetinfo called.\n",
	    ioc->name));
	/* Get the port number and set the maximum number of bytes
	 * in the returned structure.
	 * Ignore the port setting.
	 */
	numBytes = karg.hdr.maxDataSize - sizeof(mpt_ioctl_header);
	maxWordsLeft = numBytes/sizeof(int);
	port = karg.hdr.port;

	if (maxWordsLeft <= 0) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_gettargetinfo() - no memory available!\n",
			ioc->name, __FILE__, __LINE__);
		return -ENOMEM;
	}

	/* Fill in the data and return the structure to the calling
	 * program
	 */

	/* struct mpt_ioctl_targetinfo does not contain sufficient space
	 * for the target structures so when the IOCTL is called, there is
	 * not sufficient stack space for the structure. Allocate memory,
	 * populate the memory, copy back to the user, then free memory.
	 * targetInfo format:
	 * bits 31-24: reserved
	 *      23-16: LUN
	 *      15- 8: Bus Number
	 *       7- 0: Target ID
	 */
	pmem = kzalloc(numBytes, GFP_KERNEL);
	if (!pmem) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_gettargetinfo() - no memory available!\n",
			ioc->name, __FILE__, __LINE__);
		return -ENOMEM;
	}
	pdata =  (int *) pmem;

	/* Get number of devices
         */
	if (ioc->sh){
		shost_for_each_device(sdev, ioc->sh) {
			if (!maxWordsLeft)
				continue;
			vdevice = sdev->hostdata;
			if (vdevice == NULL || vdevice->vtarget == NULL)
				continue;
			if (vdevice->vtarget->tflags &
			    MPT_TARGET_FLAGS_RAID_COMPONENT)
				continue;
			lun = (vdevice->vtarget->raidVolume) ? 0x80 : vdevice->lun;
			*pdata = (((u8)lun << 16) + (vdevice->vtarget->channel << 8) +
			    (vdevice->vtarget->id ));
			pdata++;
			numDevices++;
			--maxWordsLeft;
		}
	}
	karg.numDevices = numDevices;

	/* Copy part of the data from kernel memory to user memory
	 */
	if (copy_to_user((char __user *)arg, &karg,
				sizeof(struct mpt_ioctl_targetinfo))) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_gettargetinfo - "
			"Unable to write out mpt_ioctl_targetinfo struct @ %p\n",
			ioc->name, __FILE__, __LINE__, uarg);
		kfree(pmem);
		return -EFAULT;
	}

	/* Copy the remaining data from kernel memory to user memory
	 */
	if (copy_to_user(uarg->targetInfo, pmem, numBytes)) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_gettargetinfo - "
			"Unable to write out mpt_ioctl_targetinfo struct @ %p\n",
			ioc->name, __FILE__, __LINE__, pdata);
		kfree(pmem);
		return -EFAULT;
	}

	kfree(pmem);

	return 0;
}