sg_read(struct file *filp, char __user *buf, size_t count, loff_t * ppos)
{
	Sg_device *sdp;
	Sg_fd *sfp;
	Sg_request *srp;
	int req_pack_id = -1;
	sg_io_hdr_t *hp;
	struct sg_header *old_hdr = NULL;
	int retval = 0;

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_read: count=%d\n", (int) count));

	if (!access_ok(VERIFY_WRITE, buf, count))
		return -EFAULT;
	if (sfp->force_packid && (count >= SZ_SG_HEADER)) {
		old_hdr = kmalloc(SZ_SG_HEADER, GFP_KERNEL);
		if (!old_hdr)
			return -ENOMEM;
		if (__copy_from_user(old_hdr, buf, SZ_SG_HEADER)) {
			retval = -EFAULT;
			goto free_old_hdr;
		}
		if (old_hdr->reply_len < 0) {
			if (count >= SZ_SG_IO_HDR) {
				sg_io_hdr_t *new_hdr;
				new_hdr = kmalloc(SZ_SG_IO_HDR, GFP_KERNEL);
				if (!new_hdr) {
					retval = -ENOMEM;
					goto free_old_hdr;
				}
				retval =__copy_from_user
				    (new_hdr, buf, SZ_SG_IO_HDR);
				req_pack_id = new_hdr->pack_id;
				kfree(new_hdr);
				if (retval) {
					retval = -EFAULT;
					goto free_old_hdr;
				}
			}
		} else
			req_pack_id = old_hdr->pack_id;
	}
	srp = sg_get_rq_mark(sfp, req_pack_id);
	if (!srp) {		/* now wait on packet to arrive */
		if (atomic_read(&sdp->detaching)) {
			retval = -ENODEV;
			goto free_old_hdr;
		}
		if (filp->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto free_old_hdr;
		}
		retval = wait_event_interruptible(sfp->read_wait,
			(atomic_read(&sdp->detaching) ||
			(srp = sg_get_rq_mark(sfp, req_pack_id))));
		if (atomic_read(&sdp->detaching)) {
			retval = -ENODEV;
			goto free_old_hdr;
		}
		if (retval) {
			/* -ERESTARTSYS as signal hit process */
			goto free_old_hdr;
		}
	}
	if (srp->header.interface_id != '\0') {
		retval = sg_new_read(sfp, buf, count, srp);
		goto free_old_hdr;
	}

	hp = &srp->header;
	if (old_hdr == NULL) {
		old_hdr = kmalloc(SZ_SG_HEADER, GFP_KERNEL);
		if (! old_hdr) {
			retval = -ENOMEM;
			goto free_old_hdr;
		}
	}
	memset(old_hdr, 0, SZ_SG_HEADER);
	old_hdr->reply_len = (int) hp->timeout;
	old_hdr->pack_len = old_hdr->reply_len; /* old, strange behaviour */
	old_hdr->pack_id = hp->pack_id;
	old_hdr->twelve_byte =
	    ((srp->data.cmd_opcode >= 0xc0) && (12 == hp->cmd_len)) ? 1 : 0;
	old_hdr->target_status = hp->masked_status;
	old_hdr->host_status = hp->host_status;
	old_hdr->driver_status = hp->driver_status;
	if ((CHECK_CONDITION & hp->masked_status) ||
	    (DRIVER_SENSE & hp->driver_status))
		memcpy(old_hdr->sense_buffer, srp->sense_b,
		       sizeof (old_hdr->sense_buffer));
	switch (hp->host_status) {
	/* This setup of 'result' is for backward compatibility and is best
	   ignored by the user who should use target, host + driver status */
	case DID_OK:
	case DID_PASSTHROUGH:
	case DID_SOFT_ERROR:
		old_hdr->result = 0;
		break;
	case DID_NO_CONNECT:
	case DID_BUS_BUSY:
	case DID_TIME_OUT:
		old_hdr->result = EBUSY;
		break;
	case DID_BAD_TARGET:
	case DID_ABORT:
	case DID_PARITY:
	case DID_RESET:
	case DID_BAD_INTR:
		old_hdr->result = EIO;
		break;
	case DID_ERROR:
		old_hdr->result = (srp->sense_b[0] == 0 && 
				  hp->masked_status == GOOD) ? 0 : EIO;
		break;
	default:
		old_hdr->result = EIO;
		break;
	}

	/* Now copy the result back to the user buffer.  */
	if (count >= SZ_SG_HEADER) {
		if (__copy_to_user(buf, old_hdr, SZ_SG_HEADER)) {
			retval = -EFAULT;
			goto free_old_hdr;
		}
		buf += SZ_SG_HEADER;
		if (count > old_hdr->reply_len)
			count = old_hdr->reply_len;
		if (count > SZ_SG_HEADER) {
			if (sg_read_oxfer(srp, buf, count - SZ_SG_HEADER)) {
				retval = -EFAULT;
				goto free_old_hdr;
			}
		}
	} else
		count = (old_hdr->result == 0) ? 0 : -EIO;
	sg_finish_rem_req(srp);
	sg_remove_request(sfp, srp);
	retval = count;
free_old_hdr:
	kfree(old_hdr);
	return retval;
}