static int __netlink_deliver_tap_skb(struct sk_buff *skb,
				     struct net_device *dev)
{
	struct sk_buff *nskb;
	struct sock *sk = skb->sk;
	int ret = -ENOMEM;

	dev_hold(dev);

	if (is_vmalloc_addr(skb->head))
		nskb = netlink_to_full_skb(skb, GFP_ATOMIC);
	else
		nskb = skb_clone(skb, GFP_ATOMIC);
	if (nskb) {
		nskb->dev = dev;
		nskb->protocol = htons((u16) sk->sk_protocol);
		nskb->pkt_type = netlink_is_kernel(sk) ?
				 PACKET_KERNEL : PACKET_USER;
		skb_reset_network_header(nskb);
		ret = dev_queue_xmit(nskb);
		if (unlikely(ret > 0))
			ret = net_xmit_errno(ret);
	}

	dev_put(dev);
	return ret;
}