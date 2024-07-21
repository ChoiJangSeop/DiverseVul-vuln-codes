static int add_system_zone(struct ext4_system_blocks *system_blks,
			   ext4_fsblk_t start_blk,
			   unsigned int count)
{
	struct ext4_system_zone *new_entry, *entry;
	struct rb_node **n = &system_blks->root.rb_node, *node;
	struct rb_node *parent = NULL, *new_node = NULL;

	while (*n) {
		parent = *n;
		entry = rb_entry(parent, struct ext4_system_zone, node);
		if (start_blk < entry->start_blk)
			n = &(*n)->rb_left;
		else if (start_blk >= (entry->start_blk + entry->count))
			n = &(*n)->rb_right;
		else	/* Unexpected overlap of system zones. */
			return -EFSCORRUPTED;
	}

	new_entry = kmem_cache_alloc(ext4_system_zone_cachep,
				     GFP_KERNEL);
	if (!new_entry)
		return -ENOMEM;
	new_entry->start_blk = start_blk;
	new_entry->count = count;
	new_node = &new_entry->node;

	rb_link_node(new_node, parent, n);
	rb_insert_color(new_node, &system_blks->root);

	/* Can we merge to the left? */
	node = rb_prev(new_node);
	if (node) {
		entry = rb_entry(node, struct ext4_system_zone, node);
		if (can_merge(entry, new_entry)) {
			new_entry->start_blk = entry->start_blk;
			new_entry->count += entry->count;
			rb_erase(node, &system_blks->root);
			kmem_cache_free(ext4_system_zone_cachep, entry);
		}
	}

	/* Can we merge to the right? */
	node = rb_next(new_node);
	if (node) {
		entry = rb_entry(node, struct ext4_system_zone, node);
		if (can_merge(new_entry, entry)) {
			new_entry->count += entry->count;
			rb_erase(node, &system_blks->root);
			kmem_cache_free(ext4_system_zone_cachep, entry);
		}
	}
	return 0;
}