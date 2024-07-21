mptctl_hp_hostinfo(unsigned long arg, unsigned int data_size)
{
	hp_host_info_t	__user *uarg = (void __user *) arg;
	MPT_ADAPTER		*ioc;
	struct pci_dev		*pdev;
	char                    *pbuf=NULL;
	dma_addr_t		buf_dma;
	hp_host_info_t		karg;
	CONFIGPARMS		cfg;
	ConfigPageHeader_t	hdr;
	int			iocnum;
	int			rc, cim_rev;
	ToolboxIstwiReadWriteRequest_t	*IstwiRWRequest;
	MPT_FRAME_HDR		*mf = NULL;
	unsigned long		timeleft;
	int			retval;
	u32			msgcontext;

	/* Reset long to int. Should affect IA64 and SPARC only
	 */
	if (data_size == sizeof(hp_host_info_t))
		cim_rev = 1;
	else if (data_size == sizeof(hp_host_info_rev0_t))
		cim_rev = 0;	/* obsolete */
	else
		return -EFAULT;

	if (copy_from_user(&karg, uarg, sizeof(hp_host_info_t))) {
		printk(KERN_ERR MYNAM "%s@%d::mptctl_hp_host_info - "
			"Unable to read in hp_host_info struct @ %p\n",
				__FILE__, __LINE__, uarg);
		return -EFAULT;
	}

	if (((iocnum = mpt_verify_adapter(karg.hdr.iocnum, &ioc)) < 0) ||
	    (ioc == NULL)) {
		printk(KERN_DEBUG MYNAM "%s::mptctl_hp_hostinfo() @%d - ioc%d not found!\n",
				__FILE__, __LINE__, iocnum);
		return -ENODEV;
	}
	dctlprintk(ioc, printk(MYIOC_s_DEBUG_FMT ": mptctl_hp_hostinfo called.\n",
	    ioc->name));

	/* Fill in the data and return the structure to the calling
	 * program
	 */
	pdev = (struct pci_dev *) ioc->pcidev;

	karg.vendor = pdev->vendor;
	karg.device = pdev->device;
	karg.subsystem_id = pdev->subsystem_device;
	karg.subsystem_vendor = pdev->subsystem_vendor;
	karg.devfn = pdev->devfn;
	karg.bus = pdev->bus->number;

	/* Save the SCSI host no. if
	 * SCSI driver loaded
	 */
	if (ioc->sh != NULL)
		karg.host_no = ioc->sh->host_no;
	else
		karg.host_no =  -1;

	/* Reformat the fw_version into a string */
	snprintf(karg.fw_version, sizeof(karg.fw_version),
		 "%.2hhu.%.2hhu.%.2hhu.%.2hhu",
		 ioc->facts.FWVersion.Struct.Major,
		 ioc->facts.FWVersion.Struct.Minor,
		 ioc->facts.FWVersion.Struct.Unit,
		 ioc->facts.FWVersion.Struct.Dev);

	/* Issue a config request to get the device serial number
	 */
	hdr.PageVersion = 0;
	hdr.PageLength = 0;
	hdr.PageNumber = 0;
	hdr.PageType = MPI_CONFIG_PAGETYPE_MANUFACTURING;
	cfg.cfghdr.hdr = &hdr;
	cfg.physAddr = -1;
	cfg.pageAddr = 0;
	cfg.action = MPI_CONFIG_ACTION_PAGE_HEADER;
	cfg.dir = 0;	/* read */
	cfg.timeout = 10;

	strncpy(karg.serial_number, " ", 24);
	if (mpt_config(ioc, &cfg) == 0) {
		if (cfg.cfghdr.hdr->PageLength > 0) {
			/* Issue the second config page request */
			cfg.action = MPI_CONFIG_ACTION_PAGE_READ_CURRENT;

			pbuf = pci_alloc_consistent(ioc->pcidev, hdr.PageLength * 4, &buf_dma);
			if (pbuf) {
				cfg.physAddr = buf_dma;
				if (mpt_config(ioc, &cfg) == 0) {
					ManufacturingPage0_t *pdata = (ManufacturingPage0_t *) pbuf;
					if (strlen(pdata->BoardTracerNumber) > 1) {
						strlcpy(karg.serial_number,
							pdata->BoardTracerNumber, 24);
					}
				}
				pci_free_consistent(ioc->pcidev, hdr.PageLength * 4, pbuf, buf_dma);
				pbuf = NULL;
			}
		}
	}
	rc = mpt_GetIocState(ioc, 1);
	switch (rc) {
	case MPI_IOC_STATE_OPERATIONAL:
		karg.ioc_status =  HP_STATUS_OK;
		break;

	case MPI_IOC_STATE_FAULT:
		karg.ioc_status =  HP_STATUS_FAILED;
		break;

	case MPI_IOC_STATE_RESET:
	case MPI_IOC_STATE_READY:
	default:
		karg.ioc_status =  HP_STATUS_OTHER;
		break;
	}

	karg.base_io_addr = pci_resource_start(pdev, 0);

	if ((ioc->bus_type == SAS) || (ioc->bus_type == FC))
		karg.bus_phys_width = HP_BUS_WIDTH_UNK;
	else
		karg.bus_phys_width = HP_BUS_WIDTH_16;

	karg.hard_resets = 0;
	karg.soft_resets = 0;
	karg.timeouts = 0;
	if (ioc->sh != NULL) {
		MPT_SCSI_HOST *hd =  shost_priv(ioc->sh);

		if (hd && (cim_rev == 1)) {
			karg.hard_resets = ioc->hard_resets;
			karg.soft_resets = ioc->soft_resets;
			karg.timeouts = ioc->timeouts;
		}
	}

	/* 
	 * Gather ISTWI(Industry Standard Two Wire Interface) Data
	 */
	if ((mf = mpt_get_msg_frame(mptctl_id, ioc)) == NULL) {
		dfailprintk(ioc, printk(MYIOC_s_WARN_FMT
			"%s, no msg frames!!\n", ioc->name, __func__));
		goto out;
	}

	IstwiRWRequest = (ToolboxIstwiReadWriteRequest_t *)mf;
	msgcontext = IstwiRWRequest->MsgContext;
	memset(IstwiRWRequest,0,sizeof(ToolboxIstwiReadWriteRequest_t));
	IstwiRWRequest->MsgContext = msgcontext;
	IstwiRWRequest->Function = MPI_FUNCTION_TOOLBOX;
	IstwiRWRequest->Tool = MPI_TOOLBOX_ISTWI_READ_WRITE_TOOL;
	IstwiRWRequest->Flags = MPI_TB_ISTWI_FLAGS_READ;
	IstwiRWRequest->NumAddressBytes = 0x01;
	IstwiRWRequest->DataLength = cpu_to_le16(0x04);
	if (pdev->devfn & 1)
		IstwiRWRequest->DeviceAddr = 0xB2;
	else
		IstwiRWRequest->DeviceAddr = 0xB0;

	pbuf = pci_alloc_consistent(ioc->pcidev, 4, &buf_dma);
	if (!pbuf)
		goto out;
	ioc->add_sge((char *)&IstwiRWRequest->SGL,
	    (MPT_SGE_FLAGS_SSIMPLE_READ|4), buf_dma);

	retval = 0;
	SET_MGMT_MSG_CONTEXT(ioc->ioctl_cmds.msg_context,
				IstwiRWRequest->MsgContext);
	INITIALIZE_MGMT_STATUS(ioc->ioctl_cmds.status)
	mpt_put_msg_frame(mptctl_id, ioc, mf);

retry_wait:
	timeleft = wait_for_completion_timeout(&ioc->ioctl_cmds.done,
			HZ*MPT_IOCTL_DEFAULT_TIMEOUT);
	if (!(ioc->ioctl_cmds.status & MPT_MGMT_STATUS_COMMAND_GOOD)) {
		retval = -ETIME;
		printk(MYIOC_s_WARN_FMT "%s: failed\n", ioc->name, __func__);
		if (ioc->ioctl_cmds.status & MPT_MGMT_STATUS_DID_IOCRESET) {
			mpt_free_msg_frame(ioc, mf);
			goto out;
		}
		if (!timeleft) {
			printk(MYIOC_s_WARN_FMT
			       "HOST INFO command timeout, doorbell=0x%08x\n",
			       ioc->name, mpt_GetIocState(ioc, 0));
			mptctl_timeout_expired(ioc, mf);
		} else
			goto retry_wait;
		goto out;
	}

	/*
	 *ISTWI Data Definition
	 * pbuf[0] = FW_VERSION = 0x4
	 * pbuf[1] = Bay Count = 6 or 4 or 2, depending on
	 *  the config, you should be seeing one out of these three values
	 * pbuf[2] = Drive Installed Map = bit pattern depend on which
	 *   bays have drives in them
	 * pbuf[3] = Checksum (0x100 = (byte0 + byte2 + byte3)
	 */
	if (ioc->ioctl_cmds.status & MPT_MGMT_STATUS_RF_VALID)
		karg.rsvd = *(u32 *)pbuf;

 out:
	CLEAR_MGMT_STATUS(ioc->ioctl_cmds.status)
	SET_MGMT_MSG_CONTEXT(ioc->ioctl_cmds.msg_context, 0);

	if (pbuf)
		pci_free_consistent(ioc->pcidev, 4, pbuf, buf_dma);

	/* Copy the data from kernel memory to user memory
	 */
	if (copy_to_user((char __user *)arg, &karg, sizeof(hp_host_info_t))) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_hpgethostinfo - "
			"Unable to write out hp_host_info @ %p\n",
			ioc->name, __FILE__, __LINE__, uarg);
		return -EFAULT;
	}

	return 0;

}