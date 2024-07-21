static int br_ip6_multicast_query(struct net_bridge *br,
				  struct net_bridge_port *port,
				  struct sk_buff *skb)
{
	const struct ipv6hdr *ip6h = ipv6_hdr(skb);
	struct mld_msg *mld;
	struct net_bridge_mdb_entry *mp;
	struct mld2_query *mld2q;
	struct net_bridge_port_group *p;
	struct net_bridge_port_group __rcu **pp;
	unsigned long max_delay;
	unsigned long now = jiffies;
	const struct in6_addr *group = NULL;
	int err = 0;
	u16 vid = 0;

	spin_lock(&br->multicast_lock);
	if (!netif_running(br->dev) ||
	    (port && port->state == BR_STATE_DISABLED))
		goto out;

	br_multicast_query_received(br, port, !ipv6_addr_any(&ip6h->saddr));

	if (skb->len == sizeof(*mld)) {
		if (!pskb_may_pull(skb, sizeof(*mld))) {
			err = -EINVAL;
			goto out;
		}
		mld = (struct mld_msg *) icmp6_hdr(skb);
		max_delay = msecs_to_jiffies(ntohs(mld->mld_maxdelay));
		if (max_delay)
			group = &mld->mld_mca;
	} else if (skb->len >= sizeof(*mld2q)) {
		if (!pskb_may_pull(skb, sizeof(*mld2q))) {
			err = -EINVAL;
			goto out;
		}
		mld2q = (struct mld2_query *)icmp6_hdr(skb);
		if (!mld2q->mld2q_nsrcs)
			group = &mld2q->mld2q_mca;
		max_delay = mld2q->mld2q_mrc ? MLDV2_MRC(ntohs(mld2q->mld2q_mrc)) : 1;
	}

	if (!group)
		goto out;

	br_vlan_get_tag(skb, &vid);
	mp = br_mdb_ip6_get(mlock_dereference(br->mdb, br), group, vid);
	if (!mp)
		goto out;

	max_delay *= br->multicast_last_member_count;
	if (mp->mglist &&
	    (timer_pending(&mp->timer) ?
	     time_after(mp->timer.expires, now + max_delay) :
	     try_to_del_timer_sync(&mp->timer) >= 0))
		mod_timer(&mp->timer, now + max_delay);

	for (pp = &mp->ports;
	     (p = mlock_dereference(*pp, br)) != NULL;
	     pp = &p->next) {
		if (timer_pending(&p->timer) ?
		    time_after(p->timer.expires, now + max_delay) :
		    try_to_del_timer_sync(&p->timer) >= 0)
			mod_timer(&p->timer, now + max_delay);
	}

out:
	spin_unlock(&br->multicast_lock);
	return err;
}