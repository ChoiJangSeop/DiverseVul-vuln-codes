static int br_ip4_multicast_query(struct net_bridge *br,
				  struct net_bridge_port *port,
				  struct sk_buff *skb)
{
	const struct iphdr *iph = ip_hdr(skb);
	struct igmphdr *ih = igmp_hdr(skb);
	struct net_bridge_mdb_entry *mp;
	struct igmpv3_query *ih3;
	struct net_bridge_port_group *p;
	struct net_bridge_port_group __rcu **pp;
	unsigned long max_delay;
	unsigned long now = jiffies;
	__be32 group;
	int err = 0;
	u16 vid = 0;

	spin_lock(&br->multicast_lock);
	if (!netif_running(br->dev) ||
	    (port && port->state == BR_STATE_DISABLED))
		goto out;

	br_multicast_query_received(br, port, !!iph->saddr);

	group = ih->group;

	if (skb->len == sizeof(*ih)) {
		max_delay = ih->code * (HZ / IGMP_TIMER_SCALE);

		if (!max_delay) {
			max_delay = 10 * HZ;
			group = 0;
		}
	} else {
		if (!pskb_may_pull(skb, sizeof(struct igmpv3_query))) {
			err = -EINVAL;
			goto out;
		}

		ih3 = igmpv3_query_hdr(skb);
		if (ih3->nsrcs)
			goto out;

		max_delay = ih3->code ?
			    IGMPV3_MRC(ih3->code) * (HZ / IGMP_TIMER_SCALE) : 1;
	}

	if (!group)
		goto out;

	br_vlan_get_tag(skb, &vid);
	mp = br_mdb_ip4_get(mlock_dereference(br->mdb, br), group, vid);
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