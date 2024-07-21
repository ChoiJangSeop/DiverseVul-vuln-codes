int ext4_setup_system_zone(struct super_block *sb)
{
	ext4_group_t ngroups = ext4_get_groups_count(sb);
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	struct ext4_system_blocks *system_blks;
	struct ext4_group_desc *gdp;
	ext4_group_t i;
	int flex_size = ext4_flex_bg_size(sbi);
	int ret;

	if (!test_opt(sb, BLOCK_VALIDITY)) {
		if (sbi->system_blks)
			ext4_release_system_zone(sb);
		return 0;
	}
	if (sbi->system_blks)
		return 0;

	system_blks = kzalloc(sizeof(*system_blks), GFP_KERNEL);
	if (!system_blks)
		return -ENOMEM;

	for (i=0; i < ngroups; i++) {
		cond_resched();
		if (ext4_bg_has_super(sb, i) &&
		    ((i < 5) || ((i % flex_size) == 0)))
			add_system_zone(system_blks,
					ext4_group_first_block_no(sb, i),
					ext4_bg_num_gdb(sb, i) + 1);
		gdp = ext4_get_group_desc(sb, i, NULL);
		ret = add_system_zone(system_blks,
				ext4_block_bitmap(sb, gdp), 1);
		if (ret)
			goto err;
		ret = add_system_zone(system_blks,
				ext4_inode_bitmap(sb, gdp), 1);
		if (ret)
			goto err;
		ret = add_system_zone(system_blks,
				ext4_inode_table(sb, gdp),
				sbi->s_itb_per_group);
		if (ret)
			goto err;
	}
	if (ext4_has_feature_journal(sb) && sbi->s_es->s_journal_inum) {
		ret = ext4_protect_reserved_inode(sb, system_blks,
				le32_to_cpu(sbi->s_es->s_journal_inum));
		if (ret)
			goto err;
	}

	/*
	 * System blks rbtree complete, announce it once to prevent racing
	 * with ext4_data_block_valid() accessing the rbtree at the same
	 * time.
	 */
	rcu_assign_pointer(sbi->system_blks, system_blks);

	if (test_opt(sb, DEBUG))
		debug_print_tree(sbi);
	return 0;
err:
	release_system_zone(system_blks);
	kfree(system_blks);
	return ret;
}