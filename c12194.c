static int ext4_data_block_valid_rcu(struct ext4_sb_info *sbi,
				     struct ext4_system_blocks *system_blks,
				     ext4_fsblk_t start_blk,
				     unsigned int count)
{
	struct ext4_system_zone *entry;
	struct rb_node *n;

	if ((start_blk <= le32_to_cpu(sbi->s_es->s_first_data_block)) ||
	    (start_blk + count < start_blk) ||
	    (start_blk + count > ext4_blocks_count(sbi->s_es)))
		return 0;

	if (system_blks == NULL)
		return 1;

	n = system_blks->root.rb_node;
	while (n) {
		entry = rb_entry(n, struct ext4_system_zone, node);
		if (start_blk + count - 1 < entry->start_blk)
			n = n->rb_left;
		else if (start_blk >= (entry->start_blk + entry->count))
			n = n->rb_right;
		else
			return 0;
	}
	return 1;
}