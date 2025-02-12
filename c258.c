static int blk_fill_sghdr_rq(struct request_queue *q, struct request *rq,
			     struct sg_io_hdr *hdr, fmode_t mode)
{
	if (copy_from_user(rq->cmd, hdr->cmdp, hdr->cmd_len))
		return -EFAULT;
	if (blk_verify_command(&q->cmd_filter, rq->cmd, mode & FMODE_WRITE))
		return -EPERM;

	/*
	 * fill in request structure
	 */
	rq->cmd_len = hdr->cmd_len;
	rq->cmd_type = REQ_TYPE_BLOCK_PC;

	rq->timeout = msecs_to_jiffies(hdr->timeout);
	if (!rq->timeout)
		rq->timeout = q->sg_timeout;
	if (!rq->timeout)
		rq->timeout = BLK_DEFAULT_SG_TIMEOUT;

	return 0;
}