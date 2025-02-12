static int atalk_sendmsg(struct kiocb *iocb, struct socket *sock, struct msghdr *msg,
			 int len)
{
	struct sock *sk = sock->sk;
	struct atalk_sock *at = at_sk(sk);
	struct sockaddr_at *usat = (struct sockaddr_at *)msg->msg_name;
	int flags = msg->msg_flags;
	int loopback = 0;
	struct sockaddr_at local_satalk, gsat;
	struct sk_buff *skb;
	struct net_device *dev;
	struct ddpehdr *ddp;
	int size;
	struct atalk_route *rt;
	int err;

	if (flags & ~MSG_DONTWAIT)
		return -EINVAL;

	if (len > DDP_MAXSZ)
		return -EMSGSIZE;

	if (usat) {
		if (sk->sk_zapped)
			if (atalk_autobind(sk) < 0)
				return -EBUSY;

		if (msg->msg_namelen < sizeof(*usat) ||
		    usat->sat_family != AF_APPLETALK)
			return -EINVAL;

		/* netatalk doesn't implement this check */
		if (usat->sat_addr.s_node == ATADDR_BCAST &&
		    !sock_flag(sk, SOCK_BROADCAST)) {
			printk(KERN_INFO "SO_BROADCAST: Fix your netatalk as "
					 "it will break before 2.2\n");
#if 0
			return -EPERM;
#endif
		}
	} else {
		if (sk->sk_state != TCP_ESTABLISHED)
			return -ENOTCONN;
		usat = &local_satalk;
		usat->sat_family      = AF_APPLETALK;
		usat->sat_port	      = at->dest_port;
		usat->sat_addr.s_node = at->dest_node;
		usat->sat_addr.s_net  = at->dest_net;
	}

	/* Build a packet */
	SOCK_DEBUG(sk, "SK %p: Got address.\n", sk);

	/* For headers */
	size = sizeof(struct ddpehdr) + len + ddp_dl->header_length;

	if (usat->sat_addr.s_net || usat->sat_addr.s_node == ATADDR_ANYNODE) {
		rt = atrtr_find(&usat->sat_addr);
		if (!rt)
			return -ENETUNREACH;

		dev = rt->dev;
	} else {
		struct atalk_addr at_hint;

		at_hint.s_node = 0;
		at_hint.s_net  = at->src_net;

		rt = atrtr_find(&at_hint);
		if (!rt)
			return -ENETUNREACH;

		dev = rt->dev;
	}

	SOCK_DEBUG(sk, "SK %p: Size needed %d, device %s\n",
			sk, size, dev->name);

	size += dev->hard_header_len;
	skb = sock_alloc_send_skb(sk, size, (flags & MSG_DONTWAIT), &err);
	if (!skb)
		return err;
	
	skb->sk = sk;
	skb_reserve(skb, ddp_dl->header_length);
	skb_reserve(skb, dev->hard_header_len);
	skb->dev = dev;

	SOCK_DEBUG(sk, "SK %p: Begin build.\n", sk);

	ddp = (struct ddpehdr *)skb_put(skb, sizeof(struct ddpehdr));
	ddp->deh_pad  = 0;
	ddp->deh_hops = 0;
	ddp->deh_len  = len + sizeof(*ddp);
	/*
	 * Fix up the length field [Ok this is horrible but otherwise
	 * I end up with unions of bit fields and messy bit field order
	 * compiler/endian dependencies..
	 */
	*((__u16 *)ddp) = ntohs(*((__u16 *)ddp));

	ddp->deh_dnet  = usat->sat_addr.s_net;
	ddp->deh_snet  = at->src_net;
	ddp->deh_dnode = usat->sat_addr.s_node;
	ddp->deh_snode = at->src_node;
	ddp->deh_dport = usat->sat_port;
	ddp->deh_sport = at->src_port;

	SOCK_DEBUG(sk, "SK %p: Copy user data (%d bytes).\n", sk, len);

	err = memcpy_fromiovec(skb_put(skb, len), msg->msg_iov, len);
	if (err) {
		kfree_skb(skb);
		return -EFAULT;
	}

	if (sk->sk_no_check == 1)
		ddp->deh_sum = 0;
	else
		ddp->deh_sum = atalk_checksum(ddp, len + sizeof(*ddp));

	/*
	 * Loopback broadcast packets to non gateway targets (ie routes
	 * to group we are in)
	 */
	if (ddp->deh_dnode == ATADDR_BCAST &&
	    !(rt->flags & RTF_GATEWAY) && !(dev->flags & IFF_LOOPBACK)) {
		struct sk_buff *skb2 = skb_copy(skb, GFP_KERNEL);

		if (skb2) {
			loopback = 1;
			SOCK_DEBUG(sk, "SK %p: send out(copy).\n", sk);
			if (aarp_send_ddp(dev, skb2,
					  &usat->sat_addr, NULL) == -1)
				kfree_skb(skb2);
				/* else queued/sent above in the aarp queue */
		}
	}

	if (dev->flags & IFF_LOOPBACK || loopback) {
		SOCK_DEBUG(sk, "SK %p: Loop back.\n", sk);
		/* loop back */
		skb_orphan(skb);
		ddp_dl->request(ddp_dl, skb, dev->dev_addr);
	} else {
		SOCK_DEBUG(sk, "SK %p: send out.\n", sk);
		if (rt->flags & RTF_GATEWAY) {
		    gsat.sat_addr = rt->gateway;
		    usat = &gsat;
		}

		if (aarp_send_ddp(dev, skb, &usat->sat_addr, NULL) == -1)
			kfree_skb(skb);
		/* else queued/sent above in the aarp queue */
	}
	SOCK_DEBUG(sk, "SK %p: Done write (%d).\n", sk, len);

	return len;
}