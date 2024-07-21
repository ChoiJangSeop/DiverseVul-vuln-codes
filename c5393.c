int ip6_output(struct sk_buff *skb)
{
	if (skb->len > dst_mtu(skb->dst) || dst_allfrag(skb->dst))
		return ip6_fragment(skb, ip6_output2);
	else
		return ip6_output2(skb);
}