struct net_bridge_mdb_entry *br_multicast_new_group(struct net_bridge *br,
	struct net_bridge_port *port, struct br_ip *group)
{
	struct net_bridge_mdb_htable *mdb;
	struct net_bridge_mdb_entry *mp;
	int hash;
	int err;

	mdb = rcu_dereference_protected(br->mdb, 1);
	if (!mdb) {
		err = br_mdb_rehash(&br->mdb, BR_HASH_SIZE, 0);
		if (err)
			return ERR_PTR(err);
		goto rehash;
	}

	hash = br_ip_hash(mdb, group);
	mp = br_multicast_get_group(br, port, group, hash);
	switch (PTR_ERR(mp)) {
	case 0:
		break;

	case -EAGAIN:
rehash:
		mdb = rcu_dereference_protected(br->mdb, 1);
		hash = br_ip_hash(mdb, group);
		break;

	default:
		goto out;
	}

	mp = kzalloc(sizeof(*mp), GFP_ATOMIC);
	if (unlikely(!mp))
		return ERR_PTR(-ENOMEM);

	mp->br = br;
	mp->addr = *group;
	setup_timer(&mp->timer, br_multicast_group_expired,
		    (unsigned long)mp);

	hlist_add_head_rcu(&mp->hlist[mdb->ver], &mdb->mhash[hash]);
	mdb->size++;

out:
	return mp;
}