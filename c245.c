static void untag_chunk(struct audit_chunk *chunk, struct node *p)
{
	struct audit_chunk *new;
	struct audit_tree *owner;
	int size = chunk->count - 1;
	int i, j;

	mutex_lock(&chunk->watch.inode->inotify_mutex);
	if (chunk->dead) {
		mutex_unlock(&chunk->watch.inode->inotify_mutex);
		return;
	}

	owner = p->owner;

	if (!size) {
		chunk->dead = 1;
		spin_lock(&hash_lock);
		list_del_init(&chunk->trees);
		if (owner->root == chunk)
			owner->root = NULL;
		list_del_init(&p->list);
		list_del_rcu(&chunk->hash);
		spin_unlock(&hash_lock);
		inotify_evict_watch(&chunk->watch);
		mutex_unlock(&chunk->watch.inode->inotify_mutex);
		put_inotify_watch(&chunk->watch);
		return;
	}

	new = alloc_chunk(size);
	if (!new)
		goto Fallback;
	if (inotify_clone_watch(&chunk->watch, &new->watch) < 0) {
		free_chunk(new);
		goto Fallback;
	}

	chunk->dead = 1;
	spin_lock(&hash_lock);
	list_replace_init(&chunk->trees, &new->trees);
	if (owner->root == chunk) {
		list_del_init(&owner->same_root);
		owner->root = NULL;
	}

	for (i = j = 0; i < size; i++, j++) {
		struct audit_tree *s;
		if (&chunk->owners[j] == p) {
			list_del_init(&p->list);
			i--;
			continue;
		}
		s = chunk->owners[j].owner;
		new->owners[i].owner = s;
		new->owners[i].index = chunk->owners[j].index - j + i;
		if (!s) /* result of earlier fallback */
			continue;
		get_tree(s);
		list_replace_init(&chunk->owners[i].list, &new->owners[j].list);
	}

	list_replace_rcu(&chunk->hash, &new->hash);
	list_for_each_entry(owner, &new->trees, same_root)
		owner->root = new;
	spin_unlock(&hash_lock);
	inotify_evict_watch(&chunk->watch);
	mutex_unlock(&chunk->watch.inode->inotify_mutex);
	put_inotify_watch(&chunk->watch);
	return;

Fallback:
	// do the best we can
	spin_lock(&hash_lock);
	if (owner->root == chunk) {
		list_del_init(&owner->same_root);
		owner->root = NULL;
	}
	list_del_init(&p->list);
	p->owner = NULL;
	put_tree(owner);
	spin_unlock(&hash_lock);
	mutex_unlock(&chunk->watch.inode->inotify_mutex);
}