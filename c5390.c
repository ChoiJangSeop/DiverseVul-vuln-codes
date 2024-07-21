int ip_output(struct sk_buff *skb)
{
	IP_INC_STATS(IPSTATS_MIB_OUTREQUESTS);

	if (skb->len > dst_mtu(skb->dst) && !skb_shinfo(skb)->tso_size)
		return ip_fragment(skb, ip_finish_output);
	else
		return ip_finish_output(skb);
}