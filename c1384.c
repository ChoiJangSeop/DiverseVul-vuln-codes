static void br_multicast_leave_group(struct net_bridge *br,
				     struct net_bridge_port *port,
				     struct br_ip *group)
{
	struct net_bridge_mdb_htable *mdb;
	struct net_bridge_mdb_entry *mp;
	struct net_bridge_port_group *p;
	unsigned long now;
	unsigned long time;

	spin_lock(&br->multicast_lock);
	if (!netif_running(br->dev) ||
	    (port && port->state == BR_STATE_DISABLED) ||
	    timer_pending(&br->multicast_querier_timer))
		goto out;

	mdb = mlock_dereference(br->mdb, br);
	mp = br_mdb_ip_get(mdb, group);
	if (!mp)
		goto out;

	if (port && (port->flags & BR_MULTICAST_FAST_LEAVE)) {
		struct net_bridge_port_group __rcu **pp;

		for (pp = &mp->ports;
		     (p = mlock_dereference(*pp, br)) != NULL;
		     pp = &p->next) {
			if (p->port != port)
				continue;

			rcu_assign_pointer(*pp, p->next);
			hlist_del_init(&p->mglist);
			del_timer(&p->timer);
			call_rcu_bh(&p->rcu, br_multicast_free_pg);
			br_mdb_notify(br->dev, port, group, RTM_DELMDB);

			if (!mp->ports && !mp->mglist &&
			    netif_running(br->dev))
				mod_timer(&mp->timer, jiffies);
		}
		goto out;
	}

	now = jiffies;
	time = now + br->multicast_last_member_count *
		     br->multicast_last_member_interval;

	if (!port) {
		if (mp->mglist &&
		    (timer_pending(&mp->timer) ?
		     time_after(mp->timer.expires, time) :
		     try_to_del_timer_sync(&mp->timer) >= 0)) {
			mod_timer(&mp->timer, time);
		}

		goto out;
	}

	for (p = mlock_dereference(mp->ports, br);
	     p != NULL;
	     p = mlock_dereference(p->next, br)) {
		if (p->port != port)
			continue;

		if (!hlist_unhashed(&p->mglist) &&
		    (timer_pending(&p->timer) ?
		     time_after(p->timer.expires, time) :
		     try_to_del_timer_sync(&p->timer) >= 0)) {
			mod_timer(&p->timer, time);
		}

		break;
	}

out:
	spin_unlock(&br->multicast_lock);
}