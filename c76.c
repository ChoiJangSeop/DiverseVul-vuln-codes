static int ipip6_rcv(struct sk_buff *skb)
{
	struct iphdr *iph;
	struct ip_tunnel *tunnel;

	if (!pskb_may_pull(skb, sizeof(struct ipv6hdr)))
		goto out;

	iph = ip_hdr(skb);

	read_lock(&ipip6_lock);
	if ((tunnel = ipip6_tunnel_lookup(dev_net(skb->dev),
					iph->saddr, iph->daddr)) != NULL) {
		secpath_reset(skb);
		skb->mac_header = skb->network_header;
		skb_reset_network_header(skb);
		IPCB(skb)->flags = 0;
		skb->protocol = htons(ETH_P_IPV6);
		skb->pkt_type = PACKET_HOST;

		if ((tunnel->dev->priv_flags & IFF_ISATAP) &&
		    !isatap_chksrc(skb, iph, tunnel)) {
			tunnel->stat.rx_errors++;
			read_unlock(&ipip6_lock);
			kfree_skb(skb);
			return 0;
		}
		tunnel->stat.rx_packets++;
		tunnel->stat.rx_bytes += skb->len;
		skb->dev = tunnel->dev;
		dst_release(skb->dst);
		skb->dst = NULL;
		nf_reset(skb);
		ipip6_ecn_decapsulate(iph, skb);
		netif_rx(skb);
		read_unlock(&ipip6_lock);
		return 0;
	}

	icmp_send(skb, ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, 0);
	kfree_skb(skb);
	read_unlock(&ipip6_lock);
out:
	return 0;
}