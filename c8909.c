mptctl_getiocinfo (unsigned long arg, unsigned int data_size)
{
	struct mpt_ioctl_iocinfo __user *uarg = (void __user *) arg;
	struct mpt_ioctl_iocinfo *karg;
	MPT_ADAPTER		*ioc;
	struct pci_dev		*pdev;
	int			iocnum;
	unsigned int		port;
	int			cim_rev;
	struct scsi_device 	*sdev;
	VirtDevice		*vdevice;

	/* Add of PCI INFO results in unaligned access for
	 * IA64 and Sparc. Reset long to int. Return no PCI
	 * data for obsolete format.
	 */
	if (data_size == sizeof(struct mpt_ioctl_iocinfo_rev0))
		cim_rev = 0;
	else if (data_size == sizeof(struct mpt_ioctl_iocinfo_rev1))
		cim_rev = 1;
	else if (data_size == sizeof(struct mpt_ioctl_iocinfo))
		cim_rev = 2;
	else if (data_size == (sizeof(struct mpt_ioctl_iocinfo_rev0)+12))
		cim_rev = 0;	/* obsolete */
	else
		return -EFAULT;

	karg = memdup_user(uarg, data_size);
	if (IS_ERR(karg)) {
		printk(KERN_ERR MYNAM "%s@%d::mpt_ioctl_iocinfo() - memdup_user returned error [%ld]\n",
				__FILE__, __LINE__, PTR_ERR(karg));
		return PTR_ERR(karg);
	}

	if (((iocnum = mpt_verify_adapter(karg->hdr.iocnum, &ioc)) < 0) ||
	    (ioc == NULL)) {
		printk(KERN_DEBUG MYNAM "%s::mptctl_getiocinfo() @%d - ioc%d not found!\n",
				__FILE__, __LINE__, iocnum);
		kfree(karg);
		return -ENODEV;
	}

	/* Verify the data transfer size is correct. */
	if (karg->hdr.maxDataSize != data_size) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_getiocinfo - "
			"Structure size mismatch. Command not completed.\n",
			ioc->name, __FILE__, __LINE__);
		kfree(karg);
		return -EFAULT;
	}

	dctlprintk(ioc, printk(MYIOC_s_DEBUG_FMT "mptctl_getiocinfo called.\n",
	    ioc->name));

	/* Fill in the data and return the structure to the calling
	 * program
	 */
	if (ioc->bus_type == SAS)
		karg->adapterType = MPT_IOCTL_INTERFACE_SAS;
	else if (ioc->bus_type == FC)
		karg->adapterType = MPT_IOCTL_INTERFACE_FC;
	else
		karg->adapterType = MPT_IOCTL_INTERFACE_SCSI;

	if (karg->hdr.port > 1) {
		kfree(karg);
		return -EINVAL;
	}
	port = karg->hdr.port;

	karg->port = port;
	pdev = (struct pci_dev *) ioc->pcidev;

	karg->pciId = pdev->device;
	karg->hwRev = pdev->revision;
	karg->subSystemDevice = pdev->subsystem_device;
	karg->subSystemVendor = pdev->subsystem_vendor;

	if (cim_rev == 1) {
		/* Get the PCI bus, device, and function numbers for the IOC
		 */
		karg->pciInfo.u.bits.busNumber = pdev->bus->number;
		karg->pciInfo.u.bits.deviceNumber = PCI_SLOT( pdev->devfn );
		karg->pciInfo.u.bits.functionNumber = PCI_FUNC( pdev->devfn );
	} else if (cim_rev == 2) {
		/* Get the PCI bus, device, function and segment ID numbers
		   for the IOC */
		karg->pciInfo.u.bits.busNumber = pdev->bus->number;
		karg->pciInfo.u.bits.deviceNumber = PCI_SLOT( pdev->devfn );
		karg->pciInfo.u.bits.functionNumber = PCI_FUNC( pdev->devfn );
		karg->pciInfo.segmentID = pci_domain_nr(pdev->bus);
	}

	/* Get number of devices
         */
	karg->numDevices = 0;
	if (ioc->sh) {
		shost_for_each_device(sdev, ioc->sh) {
			vdevice = sdev->hostdata;
			if (vdevice == NULL || vdevice->vtarget == NULL)
				continue;
			if (vdevice->vtarget->tflags &
			    MPT_TARGET_FLAGS_RAID_COMPONENT)
				continue;
			karg->numDevices++;
		}
	}

	/* Set the BIOS and FW Version
	 */
	karg->FWVersion = ioc->facts.FWVersion.Word;
	karg->BIOSVersion = ioc->biosVersion;

	/* Set the Version Strings.
	 */
	strncpy (karg->driverVersion, MPT_LINUX_PACKAGE_NAME, MPT_IOCTL_VERSION_LENGTH);
	karg->driverVersion[MPT_IOCTL_VERSION_LENGTH-1]='\0';

	karg->busChangeEvent = 0;
	karg->hostId = ioc->pfacts[port].PortSCSIID;
	karg->rsvd[0] = karg->rsvd[1] = 0;

	/* Copy the data from kernel memory to user memory
	 */
	if (copy_to_user((char __user *)arg, karg, data_size)) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_getiocinfo - "
			"Unable to write out mpt_ioctl_iocinfo struct @ %p\n",
			ioc->name, __FILE__, __LINE__, uarg);
		kfree(karg);
		return -EFAULT;
	}

	kfree(karg);
	return 0;
}