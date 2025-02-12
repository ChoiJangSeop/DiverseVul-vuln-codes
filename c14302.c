static int decode_getacl(struct xdr_stream *xdr, struct rpc_rqst *req,
		size_t *acl_len)
{
	__be32 *savep;
	uint32_t attrlen,
		 bitmap[3] = {0};
	struct kvec *iov = req->rq_rcv_buf.head;
	int status;

	*acl_len = 0;
	if ((status = decode_op_hdr(xdr, OP_GETATTR)) != 0)
		goto out;
	if ((status = decode_attr_bitmap(xdr, bitmap)) != 0)
		goto out;
	if ((status = decode_attr_length(xdr, &attrlen, &savep)) != 0)
		goto out;

	if (unlikely(bitmap[0] & (FATTR4_WORD0_ACL - 1U)))
		return -EIO;
	if (likely(bitmap[0] & FATTR4_WORD0_ACL)) {
		size_t hdrlen;
		u32 recvd;

		/* We ignore &savep and don't do consistency checks on
		 * the attr length.  Let userspace figure it out.... */
		hdrlen = (u8 *)xdr->p - (u8 *)iov->iov_base;
		recvd = req->rq_rcv_buf.len - hdrlen;
		if (attrlen > recvd) {
			dprintk("NFS: server cheating in getattr"
					" acl reply: attrlen %u > recvd %u\n",
					attrlen, recvd);
			return -EINVAL;
		}
		xdr_read_pages(xdr, attrlen);
		*acl_len = attrlen;
	} else
		status = -EOPNOTSUPP;

out:
	return status;
}