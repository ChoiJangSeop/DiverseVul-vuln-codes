static void trim_marked(struct audit_tree *tree)
{
	struct list_head *p, *q;
	spin_lock(&hash_lock);
	if (tree->goner) {
		spin_unlock(&hash_lock);
		return;
	}
	/* reorder */
	for (p = tree->chunks.next; p != &tree->chunks; p = q) {
		struct node *node = list_entry(p, struct node, list);
		q = p->next;
		if (node->index & (1U<<31)) {
			list_del_init(p);
			list_add(p, &tree->chunks);
		}
	}

	while (!list_empty(&tree->chunks)) {
		struct node *node;
		struct audit_chunk *chunk;

		node = list_entry(tree->chunks.next, struct node, list);

		/* have we run out of marked? */
		if (!(node->index & (1U<<31)))
			break;

		chunk = find_chunk(node);
		get_inotify_watch(&chunk->watch);
		spin_unlock(&hash_lock);

		untag_chunk(chunk, node);

		put_inotify_watch(&chunk->watch);
		spin_lock(&hash_lock);
	}
	if (!tree->root && !tree->goner) {
		tree->goner = 1;
		spin_unlock(&hash_lock);
		mutex_lock(&audit_filter_mutex);
		kill_rules(tree);
		list_del_init(&tree->list);
		mutex_unlock(&audit_filter_mutex);
		prune_one(tree);
	} else {
		spin_unlock(&hash_lock);
	}
}