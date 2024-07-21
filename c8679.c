static int bpf_lwt_xmit_reroute(struct sk_buff *skb)
{
	struct net_device *l3mdev = l3mdev_master_dev_rcu(skb_dst(skb)->dev);
	int oif = l3mdev ? l3mdev->ifindex : 0;
	struct dst_entry *dst = NULL;
	int err = -EAFNOSUPPORT;
	struct sock *sk;
	struct net *net;
	bool ipv4;

	if (skb->protocol == htons(ETH_P_IP))
		ipv4 = true;
	else if (skb->protocol == htons(ETH_P_IPV6))
		ipv4 = false;
	else
		goto err;

	sk = sk_to_full_sk(skb->sk);
	if (sk) {
		if (sk->sk_bound_dev_if)
			oif = sk->sk_bound_dev_if;
		net = sock_net(sk);
	} else {
		net = dev_net(skb_dst(skb)->dev);
	}

	if (ipv4) {
		struct iphdr *iph = ip_hdr(skb);
		struct flowi4 fl4 = {};
		struct rtable *rt;

		fl4.flowi4_oif = oif;
		fl4.flowi4_mark = skb->mark;
		fl4.flowi4_uid = sock_net_uid(net, sk);
		fl4.flowi4_tos = RT_TOS(iph->tos);
		fl4.flowi4_flags = FLOWI_FLAG_ANYSRC;
		fl4.flowi4_proto = iph->protocol;
		fl4.daddr = iph->daddr;
		fl4.saddr = iph->saddr;

		rt = ip_route_output_key(net, &fl4);
		if (IS_ERR(rt)) {
			err = PTR_ERR(rt);
			goto err;
		}
		dst = &rt->dst;
	} else {
		struct ipv6hdr *iph6 = ipv6_hdr(skb);
		struct flowi6 fl6 = {};

		fl6.flowi6_oif = oif;
		fl6.flowi6_mark = skb->mark;
		fl6.flowi6_uid = sock_net_uid(net, sk);
		fl6.flowlabel = ip6_flowinfo(iph6);
		fl6.flowi6_proto = iph6->nexthdr;
		fl6.daddr = iph6->daddr;
		fl6.saddr = iph6->saddr;

		err = ipv6_stub->ipv6_dst_lookup(net, skb->sk, &dst, &fl6);
		if (unlikely(err))
			goto err;
		if (IS_ERR(dst)) {
			err = PTR_ERR(dst);
			goto err;
		}
	}
	if (unlikely(dst->error)) {
		err = dst->error;
		dst_release(dst);
		goto err;
	}

	/* Although skb header was reserved in bpf_lwt_push_ip_encap(), it
	 * was done for the previous dst, so we are doing it here again, in
	 * case the new dst needs much more space. The call below is a noop
	 * if there is enough header space in skb.
	 */
	err = skb_cow_head(skb, LL_RESERVED_SPACE(dst->dev));
	if (unlikely(err))
		goto err;

	skb_dst_drop(skb);
	skb_dst_set(skb, dst);

	err = dst_output(dev_net(skb_dst(skb)->dev), skb->sk, skb);
	if (unlikely(err))
		return err;

	/* ip[6]_finish_output2 understand LWTUNNEL_XMIT_DONE */
	return LWTUNNEL_XMIT_DONE;

err:
	kfree_skb(skb);
	return err;
}