struct audit_chunk *audit_tree_lookup(const struct inode *inode)
{
	struct list_head *list = chunk_hash(inode);
	struct audit_chunk *p;

	list_for_each_entry_rcu(p, list, hash) {
		if (p->watch.inode == inode) {
			get_inotify_watch(&p->watch);
			return p;
		}
	}
	return NULL;
}