mptctl_do_mpt_command (struct mpt_ioctl_command karg, void __user *mfPtr)
{
	MPT_ADAPTER	*ioc;
	MPT_FRAME_HDR	*mf = NULL;
	MPIHeader_t	*hdr;
	char		*psge;
	struct buflist	bufIn;	/* data In buffer */
	struct buflist	bufOut; /* data Out buffer */
	dma_addr_t	dma_addr_in;
	dma_addr_t	dma_addr_out;
	int		sgSize = 0;	/* Num SG elements */
	int		iocnum, flagsLength;
	int		sz, rc = 0;
	int		msgContext;
	u16		req_idx;
	ulong 		timeout;
	unsigned long	timeleft;
	struct scsi_device *sdev;
	unsigned long	 flags;
	u8		 function;

	/* bufIn and bufOut are used for user to kernel space transfers
	 */
	bufIn.kptr = bufOut.kptr = NULL;
	bufIn.len = bufOut.len = 0;

	if (((iocnum = mpt_verify_adapter(karg.hdr.iocnum, &ioc)) < 0) ||
	    (ioc == NULL)) {
		printk(KERN_DEBUG MYNAM "%s::mptctl_do_mpt_command() @%d - ioc%d not found!\n",
				__FILE__, __LINE__, iocnum);
		return -ENODEV;
	}

	spin_lock_irqsave(&ioc->taskmgmt_lock, flags);
	if (ioc->ioc_reset_in_progress) {
		spin_unlock_irqrestore(&ioc->taskmgmt_lock, flags);
		printk(KERN_ERR MYNAM "%s@%d::mptctl_do_mpt_command - "
			"Busy with diagnostic reset\n", __FILE__, __LINE__);
		return -EBUSY;
	}
	spin_unlock_irqrestore(&ioc->taskmgmt_lock, flags);

	/* Basic sanity checks to prevent underflows or integer overflows */
	if (karg.maxReplyBytes < 0 ||
	    karg.dataInSize < 0 ||
	    karg.dataOutSize < 0 ||
	    karg.dataSgeOffset < 0 ||
	    karg.maxSenseBytes < 0 ||
	    karg.dataSgeOffset > ioc->req_sz / 4)
		return -EINVAL;

	/* Verify that the final request frame will not be too large.
	 */
	sz = karg.dataSgeOffset * 4;
	if (karg.dataInSize > 0)
		sz += ioc->SGE_size;
	if (karg.dataOutSize > 0)
		sz += ioc->SGE_size;

	if (sz > ioc->req_sz) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
			"Request frame too large (%d) maximum (%d)\n",
			ioc->name, __FILE__, __LINE__, sz, ioc->req_sz);
		return -EFAULT;
	}

	/* Get a free request frame and save the message context.
	 */
        if ((mf = mpt_get_msg_frame(mptctl_id, ioc)) == NULL)
                return -EAGAIN;

	hdr = (MPIHeader_t *) mf;
	msgContext = le32_to_cpu(hdr->MsgContext);
	req_idx = le16_to_cpu(mf->u.frame.hwhdr.msgctxu.fld.req_idx);

	/* Copy the request frame
	 * Reset the saved message context.
	 * Request frame in user space
	 */
	if (copy_from_user(mf, mfPtr, karg.dataSgeOffset * 4)) {
		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
			"Unable to read MF from mpt_ioctl_command struct @ %p\n",
			ioc->name, __FILE__, __LINE__, mfPtr);
		function = -1;
		rc = -EFAULT;
		goto done_free_mem;
	}
	hdr->MsgContext = cpu_to_le32(msgContext);
	function = hdr->Function;


	/* Verify that this request is allowed.
	 */
	dctlprintk(ioc, printk(MYIOC_s_DEBUG_FMT "sending mpi function (0x%02X), req=%p\n",
	    ioc->name, hdr->Function, mf));

	switch (function) {
	case MPI_FUNCTION_IOC_FACTS:
	case MPI_FUNCTION_PORT_FACTS:
		karg.dataOutSize  = karg.dataInSize = 0;
		break;

	case MPI_FUNCTION_CONFIG:
	{
		Config_t *config_frame;
		config_frame = (Config_t *)mf;
		dctlprintk(ioc, printk(MYIOC_s_DEBUG_FMT "\ttype=0x%02x ext_type=0x%02x "
		    "number=0x%02x action=0x%02x\n", ioc->name,
		    config_frame->Header.PageType,
		    config_frame->ExtPageType,
		    config_frame->Header.PageNumber,
		    config_frame->Action));
		break;
	}

	case MPI_FUNCTION_FC_COMMON_TRANSPORT_SEND:
	case MPI_FUNCTION_FC_EX_LINK_SRVC_SEND:
	case MPI_FUNCTION_FW_UPLOAD:
	case MPI_FUNCTION_SCSI_ENCLOSURE_PROCESSOR:
	case MPI_FUNCTION_FW_DOWNLOAD:
	case MPI_FUNCTION_FC_PRIMITIVE_SEND:
	case MPI_FUNCTION_TOOLBOX:
	case MPI_FUNCTION_SAS_IO_UNIT_CONTROL:
		break;

	case MPI_FUNCTION_SCSI_IO_REQUEST:
		if (ioc->sh) {
			SCSIIORequest_t *pScsiReq = (SCSIIORequest_t *) mf;
			int qtag = MPI_SCSIIO_CONTROL_UNTAGGED;
			int scsidir = 0;
			int dataSize;
			u32 id;

			id = (ioc->devices_per_bus == 0) ? 256 : ioc->devices_per_bus;
			if (pScsiReq->TargetID > id) {
				printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
					"Target ID out of bounds. \n",
					ioc->name, __FILE__, __LINE__);
				rc = -ENODEV;
				goto done_free_mem;
			}

			if (pScsiReq->Bus >= ioc->number_of_buses) {
				printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
					"Target Bus out of bounds. \n",
					ioc->name, __FILE__, __LINE__);
				rc = -ENODEV;
				goto done_free_mem;
			}

			pScsiReq->MsgFlags &= ~MPI_SCSIIO_MSGFLGS_SENSE_WIDTH;
			pScsiReq->MsgFlags |= mpt_msg_flags(ioc);


			/* verify that app has not requested
			 *	more sense data than driver
			 *	can provide, if so, reset this parameter
			 * set the sense buffer pointer low address
			 * update the control field to specify Q type
			 */
			if (karg.maxSenseBytes > MPT_SENSE_BUFFER_SIZE)
				pScsiReq->SenseBufferLength = MPT_SENSE_BUFFER_SIZE;
			else
				pScsiReq->SenseBufferLength = karg.maxSenseBytes;

			pScsiReq->SenseBufferLowAddr =
				cpu_to_le32(ioc->sense_buf_low_dma
				   + (req_idx * MPT_SENSE_BUFFER_ALLOC));

			shost_for_each_device(sdev, ioc->sh) {
				struct scsi_target *starget = scsi_target(sdev);
				VirtTarget *vtarget = starget->hostdata;

				if (vtarget == NULL)
					continue;

				if ((pScsiReq->TargetID == vtarget->id) &&
				    (pScsiReq->Bus == vtarget->channel) &&
				    (vtarget->tflags & MPT_TARGET_FLAGS_Q_YES))
					qtag = MPI_SCSIIO_CONTROL_SIMPLEQ;
			}

			/* Have the IOCTL driver set the direction based
			 * on the dataOutSize (ordering issue with Sparc).
			 */
			if (karg.dataOutSize > 0) {
				scsidir = MPI_SCSIIO_CONTROL_WRITE;
				dataSize = karg.dataOutSize;
			} else {
				scsidir = MPI_SCSIIO_CONTROL_READ;
				dataSize = karg.dataInSize;
			}

			pScsiReq->Control = cpu_to_le32(scsidir | qtag);
			pScsiReq->DataLength = cpu_to_le32(dataSize);


		} else {
			printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
				"SCSI driver is not loaded. \n",
				ioc->name, __FILE__, __LINE__);
			rc = -EFAULT;
			goto done_free_mem;
		}
		break;

	case MPI_FUNCTION_SMP_PASSTHROUGH:
		/* Check mf->PassthruFlags to determine if
		 * transfer is ImmediateMode or not.
		 * Immediate mode returns data in the ReplyFrame.
		 * Else, we are sending request and response data
		 * in two SGLs at the end of the mf.
		 */
		break;

	case MPI_FUNCTION_SATA_PASSTHROUGH:
		if (!ioc->sh) {
			printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
				"SCSI driver is not loaded. \n",
				ioc->name, __FILE__, __LINE__);
			rc = -EFAULT;
			goto done_free_mem;
		}
		break;

	case MPI_FUNCTION_RAID_ACTION:
		/* Just add a SGE
		 */
		break;

	case MPI_FUNCTION_RAID_SCSI_IO_PASSTHROUGH:
		if (ioc->sh) {
			SCSIIORequest_t *pScsiReq = (SCSIIORequest_t *) mf;
			int qtag = MPI_SCSIIO_CONTROL_SIMPLEQ;
			int scsidir = MPI_SCSIIO_CONTROL_READ;
			int dataSize;

			pScsiReq->MsgFlags &= ~MPI_SCSIIO_MSGFLGS_SENSE_WIDTH;
			pScsiReq->MsgFlags |= mpt_msg_flags(ioc);


			/* verify that app has not requested
			 *	more sense data than driver
			 *	can provide, if so, reset this parameter
			 * set the sense buffer pointer low address
			 * update the control field to specify Q type
			 */
			if (karg.maxSenseBytes > MPT_SENSE_BUFFER_SIZE)
				pScsiReq->SenseBufferLength = MPT_SENSE_BUFFER_SIZE;
			else
				pScsiReq->SenseBufferLength = karg.maxSenseBytes;

			pScsiReq->SenseBufferLowAddr =
				cpu_to_le32(ioc->sense_buf_low_dma
				   + (req_idx * MPT_SENSE_BUFFER_ALLOC));

			/* All commands to physical devices are tagged
			 */

			/* Have the IOCTL driver set the direction based
			 * on the dataOutSize (ordering issue with Sparc).
			 */
			if (karg.dataOutSize > 0) {
				scsidir = MPI_SCSIIO_CONTROL_WRITE;
				dataSize = karg.dataOutSize;
			} else {
				scsidir = MPI_SCSIIO_CONTROL_READ;
				dataSize = karg.dataInSize;
			}

			pScsiReq->Control = cpu_to_le32(scsidir | qtag);
			pScsiReq->DataLength = cpu_to_le32(dataSize);

		} else {
			printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
				"SCSI driver is not loaded. \n",
				ioc->name, __FILE__, __LINE__);
			rc = -EFAULT;
			goto done_free_mem;
		}
		break;

	case MPI_FUNCTION_SCSI_TASK_MGMT:
	{
		SCSITaskMgmt_t	*pScsiTm;
		pScsiTm = (SCSITaskMgmt_t *)mf;
		dctlprintk(ioc, printk(MYIOC_s_DEBUG_FMT
			"\tTaskType=0x%x MsgFlags=0x%x "
			"TaskMsgContext=0x%x id=%d channel=%d\n",
			ioc->name, pScsiTm->TaskType, le32_to_cpu
			(pScsiTm->TaskMsgContext), pScsiTm->MsgFlags,
			pScsiTm->TargetID, pScsiTm->Bus));
		break;
	}

	case MPI_FUNCTION_IOC_INIT:
		{
			IOCInit_t	*pInit = (IOCInit_t *) mf;
			u32		high_addr, sense_high;

			/* Verify that all entries in the IOC INIT match
			 * existing setup (and in LE format).
			 */
			if (sizeof(dma_addr_t) == sizeof(u64)) {
				high_addr = cpu_to_le32((u32)((u64)ioc->req_frames_dma >> 32));
				sense_high= cpu_to_le32((u32)((u64)ioc->sense_buf_pool_dma >> 32));
			} else {
				high_addr = 0;
				sense_high= 0;
			}

			if ((pInit->Flags != 0) || (pInit->MaxDevices != ioc->facts.MaxDevices) ||
				(pInit->MaxBuses != ioc->facts.MaxBuses) ||
				(pInit->ReplyFrameSize != cpu_to_le16(ioc->reply_sz)) ||
				(pInit->HostMfaHighAddr != high_addr) ||
				(pInit->SenseBufferHighAddr != sense_high)) {
				printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
					"IOC_INIT issued with 1 or more incorrect parameters. Rejected.\n",
					ioc->name, __FILE__, __LINE__);
				rc = -EFAULT;
				goto done_free_mem;
			}
		}
		break;
	default:
		/*
		 * MPI_FUNCTION_PORT_ENABLE
		 * MPI_FUNCTION_TARGET_CMD_BUFFER_POST
		 * MPI_FUNCTION_TARGET_ASSIST
		 * MPI_FUNCTION_TARGET_STATUS_SEND
		 * MPI_FUNCTION_TARGET_MODE_ABORT
		 * MPI_FUNCTION_IOC_MESSAGE_UNIT_RESET
		 * MPI_FUNCTION_IO_UNIT_RESET
		 * MPI_FUNCTION_HANDSHAKE
		 * MPI_FUNCTION_REPLY_FRAME_REMOVAL
		 * MPI_FUNCTION_EVENT_NOTIFICATION
		 *  (driver handles event notification)
		 * MPI_FUNCTION_EVENT_ACK
		 */

		/*  What to do with these???  CHECK ME!!!
			MPI_FUNCTION_FC_LINK_SRVC_BUF_POST
			MPI_FUNCTION_FC_LINK_SRVC_RSP
			MPI_FUNCTION_FC_ABORT
			MPI_FUNCTION_LAN_SEND
			MPI_FUNCTION_LAN_RECEIVE
		 	MPI_FUNCTION_LAN_RESET
		*/

		printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
			"Illegal request (function 0x%x) \n",
			ioc->name, __FILE__, __LINE__, hdr->Function);
		rc = -EFAULT;
		goto done_free_mem;
	}

	/* Add the SGL ( at most one data in SGE and one data out SGE )
	 * In the case of two SGE's - the data out (write) will always
	 * preceede the data in (read) SGE. psgList is used to free the
	 * allocated memory.
	 */
	psge = (char *) (((int *) mf) + karg.dataSgeOffset);
	flagsLength = 0;

	if (karg.dataOutSize > 0)
		sgSize ++;

	if (karg.dataInSize > 0)
		sgSize ++;

	if (sgSize > 0) {

		/* Set up the dataOut memory allocation */
		if (karg.dataOutSize > 0) {
			if (karg.dataInSize > 0) {
				flagsLength = ( MPI_SGE_FLAGS_SIMPLE_ELEMENT |
						MPI_SGE_FLAGS_END_OF_BUFFER |
						MPI_SGE_FLAGS_DIRECTION)
						<< MPI_SGE_FLAGS_SHIFT;
			} else {
				flagsLength = MPT_SGE_FLAGS_SSIMPLE_WRITE;
			}
			flagsLength |= karg.dataOutSize;
			bufOut.len = karg.dataOutSize;
			bufOut.kptr = pci_alloc_consistent(
					ioc->pcidev, bufOut.len, &dma_addr_out);

			if (bufOut.kptr == NULL) {
				rc = -ENOMEM;
				goto done_free_mem;
			} else {
				/* Set up this SGE.
				 * Copy to MF and to sglbuf
				 */
				ioc->add_sge(psge, flagsLength, dma_addr_out);
				psge += ioc->SGE_size;

				/* Copy user data to kernel space.
				 */
				if (copy_from_user(bufOut.kptr,
						karg.dataOutBufPtr,
						bufOut.len)) {
					printk(MYIOC_s_ERR_FMT
						"%s@%d::mptctl_do_mpt_command - Unable "
						"to read user data "
						"struct @ %p\n",
						ioc->name, __FILE__, __LINE__,karg.dataOutBufPtr);
					rc =  -EFAULT;
					goto done_free_mem;
				}
			}
		}

		if (karg.dataInSize > 0) {
			flagsLength = MPT_SGE_FLAGS_SSIMPLE_READ;
			flagsLength |= karg.dataInSize;

			bufIn.len = karg.dataInSize;
			bufIn.kptr = pci_alloc_consistent(ioc->pcidev,
					bufIn.len, &dma_addr_in);

			if (bufIn.kptr == NULL) {
				rc = -ENOMEM;
				goto done_free_mem;
			} else {
				/* Set up this SGE
				 * Copy to MF and to sglbuf
				 */
				ioc->add_sge(psge, flagsLength, dma_addr_in);
			}
		}
	} else  {
		/* Add a NULL SGE
		 */
		ioc->add_sge(psge, flagsLength, (dma_addr_t) -1);
	}

	SET_MGMT_MSG_CONTEXT(ioc->ioctl_cmds.msg_context, hdr->MsgContext);
	INITIALIZE_MGMT_STATUS(ioc->ioctl_cmds.status)
	if (hdr->Function == MPI_FUNCTION_SCSI_TASK_MGMT) {

		mutex_lock(&ioc->taskmgmt_cmds.mutex);
		if (mpt_set_taskmgmt_in_progress_flag(ioc) != 0) {
			mutex_unlock(&ioc->taskmgmt_cmds.mutex);
			goto done_free_mem;
		}

		DBG_DUMP_TM_REQUEST_FRAME(ioc, (u32 *)mf);

		if ((ioc->facts.IOCCapabilities & MPI_IOCFACTS_CAPABILITY_HIGH_PRI_Q) &&
		    (ioc->facts.MsgVersion >= MPI_VERSION_01_05))
			mpt_put_msg_frame_hi_pri(mptctl_id, ioc, mf);
		else {
			rc =mpt_send_handshake_request(mptctl_id, ioc,
				sizeof(SCSITaskMgmt_t), (u32*)mf, CAN_SLEEP);
			if (rc != 0) {
				dfailprintk(ioc, printk(MYIOC_s_ERR_FMT
				    "send_handshake FAILED! (ioc %p, mf %p)\n",
				    ioc->name, ioc, mf));
				mpt_clear_taskmgmt_in_progress_flag(ioc);
				rc = -ENODATA;
				mutex_unlock(&ioc->taskmgmt_cmds.mutex);
				goto done_free_mem;
			}
		}

	} else
		mpt_put_msg_frame(mptctl_id, ioc, mf);

	/* Now wait for the command to complete */
	timeout = (karg.timeout > 0) ? karg.timeout : MPT_IOCTL_DEFAULT_TIMEOUT;
retry_wait:
	timeleft = wait_for_completion_timeout(&ioc->ioctl_cmds.done,
				HZ*timeout);
	if (!(ioc->ioctl_cmds.status & MPT_MGMT_STATUS_COMMAND_GOOD)) {
		rc = -ETIME;
		dfailprintk(ioc, printk(MYIOC_s_ERR_FMT "%s: TIMED OUT!\n",
		    ioc->name, __func__));
		if (ioc->ioctl_cmds.status & MPT_MGMT_STATUS_DID_IOCRESET) {
			if (function == MPI_FUNCTION_SCSI_TASK_MGMT)
				mutex_unlock(&ioc->taskmgmt_cmds.mutex);
			goto done_free_mem;
		}
		if (!timeleft) {
			printk(MYIOC_s_WARN_FMT
			       "mpt cmd timeout, doorbell=0x%08x"
			       " function=0x%x\n",
			       ioc->name, mpt_GetIocState(ioc, 0), function);
			if (function == MPI_FUNCTION_SCSI_TASK_MGMT)
				mutex_unlock(&ioc->taskmgmt_cmds.mutex);
			mptctl_timeout_expired(ioc, mf);
			mf = NULL;
		} else
			goto retry_wait;
		goto done_free_mem;
	}

	if (function == MPI_FUNCTION_SCSI_TASK_MGMT)
		mutex_unlock(&ioc->taskmgmt_cmds.mutex);


	mf = NULL;

	/* If a valid reply frame, copy to the user.
	 * Offset 2: reply length in U32's
	 */
	if (ioc->ioctl_cmds.status & MPT_MGMT_STATUS_RF_VALID) {
		if (karg.maxReplyBytes < ioc->reply_sz) {
			sz = min(karg.maxReplyBytes,
				4*ioc->ioctl_cmds.reply[2]);
		} else {
			 sz = min(ioc->reply_sz, 4*ioc->ioctl_cmds.reply[2]);
		}
		if (sz > 0) {
			if (copy_to_user(karg.replyFrameBufPtr,
				 ioc->ioctl_cmds.reply, sz)){
				 printk(MYIOC_s_ERR_FMT
				     "%s@%d::mptctl_do_mpt_command - "
				 "Unable to write out reply frame %p\n",
				 ioc->name, __FILE__, __LINE__, karg.replyFrameBufPtr);
				 rc =  -ENODATA;
				 goto done_free_mem;
			}
		}
	}

	/* If valid sense data, copy to user.
	 */
	if (ioc->ioctl_cmds.status & MPT_MGMT_STATUS_SENSE_VALID) {
		sz = min(karg.maxSenseBytes, MPT_SENSE_BUFFER_SIZE);
		if (sz > 0) {
			if (copy_to_user(karg.senseDataPtr,
				ioc->ioctl_cmds.sense, sz)) {
				printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
				"Unable to write sense data to user %p\n",
				ioc->name, __FILE__, __LINE__,
				karg.senseDataPtr);
				rc =  -ENODATA;
				goto done_free_mem;
			}
		}
	}

	/* If the overall status is _GOOD and data in, copy data
	 * to user.
	 */
	if ((ioc->ioctl_cmds.status & MPT_MGMT_STATUS_COMMAND_GOOD) &&
				(karg.dataInSize > 0) && (bufIn.kptr)) {

		if (copy_to_user(karg.dataInBufPtr,
				 bufIn.kptr, karg.dataInSize)) {
			printk(MYIOC_s_ERR_FMT "%s@%d::mptctl_do_mpt_command - "
				"Unable to write data to user %p\n",
				ioc->name, __FILE__, __LINE__,
				karg.dataInBufPtr);
			rc =  -ENODATA;
		}
	}

done_free_mem:

	CLEAR_MGMT_STATUS(ioc->ioctl_cmds.status)
	SET_MGMT_MSG_CONTEXT(ioc->ioctl_cmds.msg_context, 0);

	/* Free the allocated memory.
	 */
	if (bufOut.kptr != NULL) {
		pci_free_consistent(ioc->pcidev,
			bufOut.len, (void *) bufOut.kptr, dma_addr_out);
	}

	if (bufIn.kptr != NULL) {
		pci_free_consistent(ioc->pcidev,
			bufIn.len, (void *) bufIn.kptr, dma_addr_in);
	}

	/* mf is null if command issued successfully
	 * otherwise, failure occurred after mf acquired.
	 */
	if (mf)
		mpt_free_msg_frame(ioc, mf);

	return rc;
}