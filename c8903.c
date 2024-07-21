mptctl_do_fw_download(int ioc, char __user *ufwbuf, size_t fwlen)
{
	FWDownload_t		*dlmsg;
	MPT_FRAME_HDR		*mf;
	MPT_ADAPTER		*iocp;
	FWDownloadTCSGE_t	*ptsge;
	MptSge_t		*sgl, *sgIn;
	char			*sgOut;
	struct buflist		*buflist;
	struct buflist		*bl;
	dma_addr_t		 sgl_dma;
	int			 ret;
	int			 numfrags = 0;
	int			 maxfrags;
	int			 n = 0;
	u32			 sgdir;
	u32			 nib;
	int			 fw_bytes_copied = 0;
	int			 i;
	int			 sge_offset = 0;
	u16			 iocstat;
	pFWDownloadReply_t	 ReplyMsg = NULL;
	unsigned long		 timeleft;

	if (mpt_verify_adapter(ioc, &iocp) < 0) {
		printk(KERN_DEBUG MYNAM "ioctl_fwdl - ioc%d not found!\n",
				 ioc);
		return -ENODEV; /* (-6) No such device or address */
	} else {

		/*  Valid device. Get a message frame and construct the FW download message.
	 	*/
		if ((mf = mpt_get_msg_frame(mptctl_id, iocp)) == NULL)
			return -EAGAIN;
	}

	dctlprintk(iocp, printk(MYIOC_s_DEBUG_FMT
	    "mptctl_do_fwdl called. mptctl_id = %xh.\n", iocp->name, mptctl_id));
	dctlprintk(iocp, printk(MYIOC_s_DEBUG_FMT "DbG: kfwdl.bufp  = %p\n",
	    iocp->name, ufwbuf));
	dctlprintk(iocp, printk(MYIOC_s_DEBUG_FMT "DbG: kfwdl.fwlen = %d\n",
	    iocp->name, (int)fwlen));
	dctlprintk(iocp, printk(MYIOC_s_DEBUG_FMT "DbG: kfwdl.ioc   = %04xh\n",
	    iocp->name, ioc));

	dlmsg = (FWDownload_t*) mf;
	ptsge = (FWDownloadTCSGE_t *) &dlmsg->SGL;
	sgOut = (char *) (ptsge + 1);

	/*
	 * Construct f/w download request
	 */
	dlmsg->ImageType = MPI_FW_DOWNLOAD_ITYPE_FW;
	dlmsg->Reserved = 0;
	dlmsg->ChainOffset = 0;
	dlmsg->Function = MPI_FUNCTION_FW_DOWNLOAD;
	dlmsg->Reserved1[0] = dlmsg->Reserved1[1] = dlmsg->Reserved1[2] = 0;
	if (iocp->facts.MsgVersion >= MPI_VERSION_01_05)
		dlmsg->MsgFlags = MPI_FW_DOWNLOAD_MSGFLGS_LAST_SEGMENT;
	else
		dlmsg->MsgFlags = 0;


	/* Set up the Transaction SGE.
	 */
	ptsge->Reserved = 0;
	ptsge->ContextSize = 0;
	ptsge->DetailsLength = 12;
	ptsge->Flags = MPI_SGE_FLAGS_TRANSACTION_ELEMENT;
	ptsge->Reserved_0100_Checksum = 0;
	ptsge->ImageOffset = 0;
	ptsge->ImageSize = cpu_to_le32(fwlen);

	/* Add the SGL
	 */

	/*
	 * Need to kmalloc area(s) for holding firmware image bytes.
	 * But we need to do it piece meal, using a proper
	 * scatter gather list (with 128kB MAX hunks).
	 *
	 * A practical limit here might be # of sg hunks that fit into
	 * a single IOC request frame; 12 or 8 (see below), so:
	 * For FC9xx: 12 x 128kB == 1.5 mB (max)
	 * For C1030:  8 x 128kB == 1   mB (max)
	 * We could support chaining, but things get ugly(ier:)
	 *
	 * Set the sge_offset to the start of the sgl (bytes).
	 */
	sgdir = 0x04000000;		/* IOC will READ from sys mem */
	sge_offset = sizeof(MPIHeader_t) + sizeof(FWDownloadTCSGE_t);
	if ((sgl = kbuf_alloc_2_sgl(fwlen, sgdir, sge_offset,
				    &numfrags, &buflist, &sgl_dma, iocp)) == NULL)
		return -ENOMEM;

	/*
	 * We should only need SGL with 2 simple_32bit entries (up to 256 kB)
	 * for FC9xx f/w image, but calculate max number of sge hunks
	 * we can fit into a request frame, and limit ourselves to that.
	 * (currently no chain support)
	 * maxfrags = (Request Size - FWdownload Size ) / Size of 32 bit SGE
	 *	Request		maxfrags
	 *	128		12
	 *	96		8
	 *	64		4
	 */
	maxfrags = (iocp->req_sz - sizeof(MPIHeader_t) -
			sizeof(FWDownloadTCSGE_t))
			/ iocp->SGE_size;
	if (numfrags > maxfrags) {
		ret = -EMLINK;
		goto fwdl_out;
	}

	dctlprintk(iocp, printk(MYIOC_s_DEBUG_FMT "DbG: sgl buffer = %p, sgfrags = %d\n",
	    iocp->name, sgl, numfrags));

	/*
	 * Parse SG list, copying sgl itself,
	 * plus f/w image hunks from user space as we go...
	 */
	ret = -EFAULT;
	sgIn = sgl;
	bl = buflist;
	for (i=0; i < numfrags; i++) {

		/* Get the SGE type: 0 - TCSGE, 3 - Chain, 1 - Simple SGE
		 * Skip everything but Simple. If simple, copy from
		 *	user space into kernel space.
		 * Note: we should not have anything but Simple as
		 *	Chain SGE are illegal.
		 */
		nib = (sgIn->FlagsLength & 0x30000000) >> 28;
		if (nib == 0 || nib == 3) {
			;
		} else if (sgIn->Address) {
			iocp->add_sge(sgOut, sgIn->FlagsLength, sgIn->Address);
			n++;
			if (copy_from_user(bl->kptr, ufwbuf+fw_bytes_copied, bl->len)) {
				printk(MYIOC_s_ERR_FMT "%s@%d::_ioctl_fwdl - "
					"Unable to copy f/w buffer hunk#%d @ %p\n",
					iocp->name, __FILE__, __LINE__, n, ufwbuf);
				goto fwdl_out;
			}
			fw_bytes_copied += bl->len;
		}
		sgIn++;
		bl++;
		sgOut += iocp->SGE_size;
	}

	DBG_DUMP_FW_DOWNLOAD(iocp, (u32 *)mf, numfrags);

	/*
	 * Finally, perform firmware download.
	 */
	ReplyMsg = NULL;
	SET_MGMT_MSG_CONTEXT(iocp->ioctl_cmds.msg_context, dlmsg->MsgContext);
	INITIALIZE_MGMT_STATUS(iocp->ioctl_cmds.status)
	mpt_put_msg_frame(mptctl_id, iocp, mf);

	/* Now wait for the command to complete */
retry_wait:
	timeleft = wait_for_completion_timeout(&iocp->ioctl_cmds.done, HZ*60);
	if (!(iocp->ioctl_cmds.status & MPT_MGMT_STATUS_COMMAND_GOOD)) {
		ret = -ETIME;
		printk(MYIOC_s_WARN_FMT "%s: failed\n", iocp->name, __func__);
		if (iocp->ioctl_cmds.status & MPT_MGMT_STATUS_DID_IOCRESET) {
			mpt_free_msg_frame(iocp, mf);
			goto fwdl_out;
		}
		if (!timeleft) {
			printk(MYIOC_s_WARN_FMT
			       "FW download timeout, doorbell=0x%08x\n",
			       iocp->name, mpt_GetIocState(iocp, 0));
			mptctl_timeout_expired(iocp, mf);
		} else
			goto retry_wait;
		goto fwdl_out;
	}

	if (!(iocp->ioctl_cmds.status & MPT_MGMT_STATUS_RF_VALID)) {
		printk(MYIOC_s_WARN_FMT "%s: failed\n", iocp->name, __func__);
		mpt_free_msg_frame(iocp, mf);
		ret = -ENODATA;
		goto fwdl_out;
	}

	if (sgl)
		kfree_sgl(sgl, sgl_dma, buflist, iocp);

	ReplyMsg = (pFWDownloadReply_t)iocp->ioctl_cmds.reply;
	iocstat = le16_to_cpu(ReplyMsg->IOCStatus) & MPI_IOCSTATUS_MASK;
	if (iocstat == MPI_IOCSTATUS_SUCCESS) {
		printk(MYIOC_s_INFO_FMT "F/W update successful!\n", iocp->name);
		return 0;
	} else if (iocstat == MPI_IOCSTATUS_INVALID_FUNCTION) {
		printk(MYIOC_s_WARN_FMT "Hmmm...  F/W download not supported!?!\n",
			iocp->name);
		printk(MYIOC_s_WARN_FMT "(time to go bang on somebodies door)\n",
			iocp->name);
		return -EBADRQC;
	} else if (iocstat == MPI_IOCSTATUS_BUSY) {
		printk(MYIOC_s_WARN_FMT "IOC_BUSY!\n", iocp->name);
		printk(MYIOC_s_WARN_FMT "(try again later?)\n", iocp->name);
		return -EBUSY;
	} else {
		printk(MYIOC_s_WARN_FMT "ioctl_fwdl() returned [bad] status = %04xh\n",
			iocp->name, iocstat);
		printk(MYIOC_s_WARN_FMT "(bad VooDoo)\n", iocp->name);
		return -ENOMSG;
	}
	return 0;

fwdl_out:

	CLEAR_MGMT_STATUS(iocp->ioctl_cmds.status);
	SET_MGMT_MSG_CONTEXT(iocp->ioctl_cmds.msg_context, 0);
        kfree_sgl(sgl, sgl_dma, buflist, iocp);
	return ret;
}