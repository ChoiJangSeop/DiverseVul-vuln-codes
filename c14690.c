static int __sock_diag_rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh)
{
	int err;
	struct sock_diag_req *req = nlmsg_data(nlh);
	const struct sock_diag_handler *hndl;

	if (nlmsg_len(nlh) < sizeof(*req))
		return -EINVAL;

	hndl = sock_diag_lock_handler(req->sdiag_family);
	if (hndl == NULL)
		err = -ENOENT;
	else
		err = hndl->dump(skb, nlh);
	sock_diag_unlock_handler(hndl);

	return err;
}