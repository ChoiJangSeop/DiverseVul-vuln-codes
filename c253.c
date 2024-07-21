static void __free_chunk(struct rcu_head *rcu)
{
	struct audit_chunk *chunk = container_of(rcu, struct audit_chunk, head);
	int i;

	for (i = 0; i < chunk->count; i++) {
		if (chunk->owners[i].owner)
			put_tree(chunk->owners[i].owner);
	}
	kfree(chunk);
}