 */
bool skb_gso_validate_mtu(const struct sk_buff *skb, unsigned int mtu)
{
	const struct skb_shared_info *shinfo = skb_shinfo(skb);
	const struct sk_buff *iter;
	unsigned int hlen;

	hlen = skb_gso_network_seglen(skb);

	if (shinfo->gso_size != GSO_BY_FRAGS)
		return hlen <= mtu;

	/* Undo this so we can re-use header sizes */
	hlen -= GSO_BY_FRAGS;

	skb_walk_frags(skb, iter) {
		if (hlen + skb_headlen(iter) > mtu)
			return false;
	}

	return true;