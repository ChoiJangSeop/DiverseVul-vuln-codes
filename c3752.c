int propagate_mnt(struct mount *dest_mnt, struct mountpoint *dest_mp,
		    struct mount *source_mnt, struct hlist_head *tree_list)
{
	struct mount *m, *n;
	int ret = 0;

	/*
	 * we don't want to bother passing tons of arguments to
	 * propagate_one(); everything is serialized by namespace_sem,
	 * so globals will do just fine.
	 */
	user_ns = current->nsproxy->mnt_ns->user_ns;
	last_dest = dest_mnt;
	last_source = source_mnt;
	mp = dest_mp;
	list = tree_list;
	dest_master = dest_mnt->mnt_master;

	/* all peers of dest_mnt, except dest_mnt itself */
	for (n = next_peer(dest_mnt); n != dest_mnt; n = next_peer(n)) {
		ret = propagate_one(n);
		if (ret)
			goto out;
	}

	/* all slave groups */
	for (m = next_group(dest_mnt, dest_mnt); m;
			m = next_group(m, dest_mnt)) {
		/* everything in that slave group */
		n = m;
		do {
			ret = propagate_one(n);
			if (ret)
				goto out;
			n = next_peer(n);
		} while (n != m);
	}
out:
	read_seqlock_excl(&mount_lock);
	hlist_for_each_entry(n, tree_list, mnt_hash) {
		m = n->mnt_parent;
		if (m->mnt_master != dest_mnt->mnt_master)
			CLEAR_MNT_MARK(m->mnt_master);
	}
	read_sequnlock_excl(&mount_lock);
	return ret;
}