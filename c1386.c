static int br_multicast_add_group(struct net_bridge *br,
				  struct net_bridge_port *port,
				  struct br_ip *group)
{
	struct net_bridge_mdb_entry *mp;
	struct net_bridge_port_group *p;
	struct net_bridge_port_group __rcu **pp;
	unsigned long now = jiffies;
	int err;

	spin_lock(&br->multicast_lock);
	if (!netif_running(br->dev) ||
	    (port && port->state == BR_STATE_DISABLED))
		goto out;

	mp = br_multicast_new_group(br, port, group);
	err = PTR_ERR(mp);
	if (IS_ERR(mp))
		goto err;

	if (!port) {
		mp->mglist = true;
		mod_timer(&mp->timer, now + br->multicast_membership_interval);
		goto out;
	}

	for (pp = &mp->ports;
	     (p = mlock_dereference(*pp, br)) != NULL;
	     pp = &p->next) {
		if (p->port == port)
			goto found;
		if ((unsigned long)p->port < (unsigned long)port)
			break;
	}

	p = br_multicast_new_port_group(port, group, *pp, MDB_TEMPORARY);
	if (unlikely(!p))
		goto err;
	rcu_assign_pointer(*pp, p);
	br_mdb_notify(br->dev, port, group, RTM_NEWMDB);

found:
	mod_timer(&p->timer, now + br->multicast_membership_interval);
out:
	err = 0;

err:
	spin_unlock(&br->multicast_lock);
	return err;
}