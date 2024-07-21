static void prune_one(struct audit_tree *victim)
{
	spin_lock(&hash_lock);
	while (!list_empty(&victim->chunks)) {
		struct node *p;
		struct audit_chunk *chunk;

		p = list_entry(victim->chunks.next, struct node, list);
		chunk = find_chunk(p);
		get_inotify_watch(&chunk->watch);
		spin_unlock(&hash_lock);

		untag_chunk(chunk, p);

		put_inotify_watch(&chunk->watch);
		spin_lock(&hash_lock);
	}
	spin_unlock(&hash_lock);
	put_tree(victim);
}