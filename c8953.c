sg_write(struct file *filp, const char __user *buf, size_t count, loff_t * ppos)
{
	int mxsize, cmd_size, k;
	int input_size, blocking;
	unsigned char opcode;
	Sg_device *sdp;
	Sg_fd *sfp;
	Sg_request *srp;
	struct sg_header old_hdr;
	sg_io_hdr_t *hp;
	unsigned char cmnd[SG_MAX_CDB_SIZE];
	int retval;

	retval = sg_check_file_access(filp, __func__);
	if (retval)
		return retval;

	if ((!(sfp = (Sg_fd *) filp->private_data)) || (!(sdp = sfp->parentdp)))
		return -ENXIO;
	SCSI_LOG_TIMEOUT(3, sg_printk(KERN_INFO, sdp,
				      "sg_write: count=%d\n", (int) count));
	if (atomic_read(&sdp->detaching))
		return -ENODEV;
	if (!((filp->f_flags & O_NONBLOCK) ||
	      scsi_block_when_processing_errors(sdp->device)))
		return -ENXIO;

	if (count < SZ_SG_HEADER)
		return -EIO;
	if (copy_from_user(&old_hdr, buf, SZ_SG_HEADER))
		return -EFAULT;
	blocking = !(filp->f_flags & O_NONBLOCK);
	if (old_hdr.reply_len < 0)
		return sg_new_write(sfp, filp, buf, count,
				    blocking, 0, 0, NULL);
	if (count < (SZ_SG_HEADER + 6))
		return -EIO;	/* The minimum scsi command length is 6 bytes. */

	buf += SZ_SG_HEADER;
	if (get_user(opcode, buf))
		return -EFAULT;

	if (!(srp = sg_add_request(sfp))) {
		SCSI_LOG_TIMEOUT(1, sg_printk(KERN_INFO, sdp,
					      "sg_write: queue full\n"));
		return -EDOM;
	}
	mutex_lock(&sfp->f_mutex);
	if (sfp->next_cmd_len > 0) {
		cmd_size = sfp->next_cmd_len;
		sfp->next_cmd_len = 0;	/* reset so only this write() effected */
	} else {
		cmd_size = COMMAND_SIZE(opcode);	/* based on SCSI command group */
		if ((opcode >= 0xc0) && old_hdr.twelve_byte)
			cmd_size = 12;
	}
	mutex_unlock(&sfp->f_mutex);
	SCSI_LOG_TIMEOUT(4, sg_printk(KERN_INFO, sdp,
		"sg_write:   scsi opcode=0x%02x, cmd_size=%d\n", (int) opcode, cmd_size));
/* Determine buffer size.  */
	input_size = count - cmd_size;
	mxsize = (input_size > old_hdr.reply_len) ? input_size : old_hdr.reply_len;
	mxsize -= SZ_SG_HEADER;
	input_size -= SZ_SG_HEADER;
	if (input_size < 0) {
		sg_remove_request(sfp, srp);
		return -EIO;	/* User did not pass enough bytes for this command. */
	}
	hp = &srp->header;
	hp->interface_id = '\0';	/* indicator of old interface tunnelled */
	hp->cmd_len = (unsigned char) cmd_size;
	hp->iovec_count = 0;
	hp->mx_sb_len = 0;
	if (input_size > 0)
		hp->dxfer_direction = (old_hdr.reply_len > SZ_SG_HEADER) ?
		    SG_DXFER_TO_FROM_DEV : SG_DXFER_TO_DEV;
	else
		hp->dxfer_direction = (mxsize > 0) ? SG_DXFER_FROM_DEV : SG_DXFER_NONE;
	hp->dxfer_len = mxsize;
	if ((hp->dxfer_direction == SG_DXFER_TO_DEV) ||
	    (hp->dxfer_direction == SG_DXFER_TO_FROM_DEV))
		hp->dxferp = (char __user *)buf + cmd_size;
	else
		hp->dxferp = NULL;
	hp->sbp = NULL;
	hp->timeout = old_hdr.reply_len;	/* structure abuse ... */
	hp->flags = input_size;	/* structure abuse ... */
	hp->pack_id = old_hdr.pack_id;
	hp->usr_ptr = NULL;
	if (copy_from_user(cmnd, buf, cmd_size))
		return -EFAULT;
	/*
	 * SG_DXFER_TO_FROM_DEV is functionally equivalent to SG_DXFER_FROM_DEV,
	 * but is is possible that the app intended SG_DXFER_TO_DEV, because there
	 * is a non-zero input_size, so emit a warning.
	 */
	if (hp->dxfer_direction == SG_DXFER_TO_FROM_DEV) {
		printk_ratelimited(KERN_WARNING
				   "sg_write: data in/out %d/%d bytes "
				   "for SCSI command 0x%x-- guessing "
				   "data in;\n   program %s not setting "
				   "count and/or reply_len properly\n",
				   old_hdr.reply_len - (int)SZ_SG_HEADER,
				   input_size, (unsigned int) cmnd[0],
				   current->comm);
	}
	k = sg_common_write(sfp, srp, cmnd, sfp->timeout, blocking);
	return (k < 0) ? k : count;
}