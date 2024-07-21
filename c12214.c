static int vmci_transport_dgram_dequeue(struct kiocb *kiocb,
					struct vsock_sock *vsk,
					struct msghdr *msg, size_t len,
					int flags)
{
	int err;
	int noblock;
	struct vmci_datagram *dg;
	size_t payload_len;
	struct sk_buff *skb;

	noblock = flags & MSG_DONTWAIT;

	if (flags & MSG_OOB || flags & MSG_ERRQUEUE)
		return -EOPNOTSUPP;

	msg->msg_namelen = 0;

	/* Retrieve the head sk_buff from the socket's receive queue. */
	err = 0;
	skb = skb_recv_datagram(&vsk->sk, flags, noblock, &err);
	if (err)
		return err;

	if (!skb)
		return -EAGAIN;

	dg = (struct vmci_datagram *)skb->data;
	if (!dg)
		/* err is 0, meaning we read zero bytes. */
		goto out;

	payload_len = dg->payload_size;
	/* Ensure the sk_buff matches the payload size claimed in the packet. */
	if (payload_len != skb->len - sizeof(*dg)) {
		err = -EINVAL;
		goto out;
	}

	if (payload_len > len) {
		payload_len = len;
		msg->msg_flags |= MSG_TRUNC;
	}

	/* Place the datagram payload in the user's iovec. */
	err = skb_copy_datagram_iovec(skb, sizeof(*dg), msg->msg_iov,
		payload_len);
	if (err)
		goto out;

	if (msg->msg_name) {
		struct sockaddr_vm *vm_addr;

		/* Provide the address of the sender. */
		vm_addr = (struct sockaddr_vm *)msg->msg_name;
		vsock_addr_init(vm_addr, dg->src.context, dg->src.resource);
		msg->msg_namelen = sizeof(*vm_addr);
	}
	err = payload_len;

out:
	skb_free_datagram(&vsk->sk, skb);
	return err;
}