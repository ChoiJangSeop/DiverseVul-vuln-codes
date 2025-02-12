static ssize_t qib_write(struct file *fp, const char __user *data,
			 size_t count, loff_t *off)
{
	const struct qib_cmd __user *ucmd;
	struct qib_ctxtdata *rcd;
	const void __user *src;
	size_t consumed, copy = 0;
	struct qib_cmd cmd;
	ssize_t ret = 0;
	void *dest;

	if (count < sizeof(cmd.type)) {
		ret = -EINVAL;
		goto bail;
	}

	ucmd = (const struct qib_cmd __user *) data;

	if (copy_from_user(&cmd.type, &ucmd->type, sizeof(cmd.type))) {
		ret = -EFAULT;
		goto bail;
	}

	consumed = sizeof(cmd.type);

	switch (cmd.type) {
	case QIB_CMD_ASSIGN_CTXT:
	case QIB_CMD_USER_INIT:
		copy = sizeof(cmd.cmd.user_info);
		dest = &cmd.cmd.user_info;
		src = &ucmd->cmd.user_info;
		break;

	case QIB_CMD_RECV_CTRL:
		copy = sizeof(cmd.cmd.recv_ctrl);
		dest = &cmd.cmd.recv_ctrl;
		src = &ucmd->cmd.recv_ctrl;
		break;

	case QIB_CMD_CTXT_INFO:
		copy = sizeof(cmd.cmd.ctxt_info);
		dest = &cmd.cmd.ctxt_info;
		src = &ucmd->cmd.ctxt_info;
		break;

	case QIB_CMD_TID_UPDATE:
	case QIB_CMD_TID_FREE:
		copy = sizeof(cmd.cmd.tid_info);
		dest = &cmd.cmd.tid_info;
		src = &ucmd->cmd.tid_info;
		break;

	case QIB_CMD_SET_PART_KEY:
		copy = sizeof(cmd.cmd.part_key);
		dest = &cmd.cmd.part_key;
		src = &ucmd->cmd.part_key;
		break;

	case QIB_CMD_DISARM_BUFS:
	case QIB_CMD_PIOAVAILUPD: /* force an update of PIOAvail reg */
		copy = 0;
		src = NULL;
		dest = NULL;
		break;

	case QIB_CMD_POLL_TYPE:
		copy = sizeof(cmd.cmd.poll_type);
		dest = &cmd.cmd.poll_type;
		src = &ucmd->cmd.poll_type;
		break;

	case QIB_CMD_ARMLAUNCH_CTRL:
		copy = sizeof(cmd.cmd.armlaunch_ctrl);
		dest = &cmd.cmd.armlaunch_ctrl;
		src = &ucmd->cmd.armlaunch_ctrl;
		break;

	case QIB_CMD_SDMA_INFLIGHT:
		copy = sizeof(cmd.cmd.sdma_inflight);
		dest = &cmd.cmd.sdma_inflight;
		src = &ucmd->cmd.sdma_inflight;
		break;

	case QIB_CMD_SDMA_COMPLETE:
		copy = sizeof(cmd.cmd.sdma_complete);
		dest = &cmd.cmd.sdma_complete;
		src = &ucmd->cmd.sdma_complete;
		break;

	case QIB_CMD_ACK_EVENT:
		copy = sizeof(cmd.cmd.event_mask);
		dest = &cmd.cmd.event_mask;
		src = &ucmd->cmd.event_mask;
		break;

	default:
		ret = -EINVAL;
		goto bail;
	}

	if (copy) {
		if ((count - consumed) < copy) {
			ret = -EINVAL;
			goto bail;
		}
		if (copy_from_user(dest, src, copy)) {
			ret = -EFAULT;
			goto bail;
		}
		consumed += copy;
	}

	rcd = ctxt_fp(fp);
	if (!rcd && cmd.type != QIB_CMD_ASSIGN_CTXT) {
		ret = -EINVAL;
		goto bail;
	}

	switch (cmd.type) {
	case QIB_CMD_ASSIGN_CTXT:
		ret = qib_assign_ctxt(fp, &cmd.cmd.user_info);
		if (ret)
			goto bail;
		break;

	case QIB_CMD_USER_INIT:
		ret = qib_do_user_init(fp, &cmd.cmd.user_info);
		if (ret)
			goto bail;
		ret = qib_get_base_info(fp, (void __user *) (unsigned long)
					cmd.cmd.user_info.spu_base_info,
					cmd.cmd.user_info.spu_base_info_size);
		break;

	case QIB_CMD_RECV_CTRL:
		ret = qib_manage_rcvq(rcd, subctxt_fp(fp), cmd.cmd.recv_ctrl);
		break;

	case QIB_CMD_CTXT_INFO:
		ret = qib_ctxt_info(fp, (struct qib_ctxt_info __user *)
				    (unsigned long) cmd.cmd.ctxt_info);
		break;

	case QIB_CMD_TID_UPDATE:
		ret = qib_tid_update(rcd, fp, &cmd.cmd.tid_info);
		break;

	case QIB_CMD_TID_FREE:
		ret = qib_tid_free(rcd, subctxt_fp(fp), &cmd.cmd.tid_info);
		break;

	case QIB_CMD_SET_PART_KEY:
		ret = qib_set_part_key(rcd, cmd.cmd.part_key);
		break;

	case QIB_CMD_DISARM_BUFS:
		(void)qib_disarm_piobufs_ifneeded(rcd);
		ret = disarm_req_delay(rcd);
		break;

	case QIB_CMD_PIOAVAILUPD:
		qib_force_pio_avail_update(rcd->dd);
		break;

	case QIB_CMD_POLL_TYPE:
		rcd->poll_type = cmd.cmd.poll_type;
		break;

	case QIB_CMD_ARMLAUNCH_CTRL:
		rcd->dd->f_set_armlaunch(rcd->dd, cmd.cmd.armlaunch_ctrl);
		break;

	case QIB_CMD_SDMA_INFLIGHT:
		ret = qib_sdma_get_inflight(user_sdma_queue_fp(fp),
					    (u32 __user *) (unsigned long)
					    cmd.cmd.sdma_inflight);
		break;

	case QIB_CMD_SDMA_COMPLETE:
		ret = qib_sdma_get_complete(rcd->ppd,
					    user_sdma_queue_fp(fp),
					    (u32 __user *) (unsigned long)
					    cmd.cmd.sdma_complete);
		break;

	case QIB_CMD_ACK_EVENT:
		ret = qib_user_event_ack(rcd, subctxt_fp(fp),
					 cmd.cmd.event_mask);
		break;
	}

	if (ret >= 0)
		ret = consumed;

bail:
	return ret;
}