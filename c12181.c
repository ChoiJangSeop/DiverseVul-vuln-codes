static int ext4_protect_reserved_inode(struct super_block *sb,
				       struct ext4_system_blocks *system_blks,
				       u32 ino)
{
	struct inode *inode;
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	struct ext4_map_blocks map;
	u32 i = 0, num;
	int err = 0, n;

	if ((ino < EXT4_ROOT_INO) ||
	    (ino > le32_to_cpu(sbi->s_es->s_inodes_count)))
		return -EINVAL;
	inode = ext4_iget(sb, ino, EXT4_IGET_SPECIAL);
	if (IS_ERR(inode))
		return PTR_ERR(inode);
	num = (inode->i_size + sb->s_blocksize - 1) >> sb->s_blocksize_bits;
	while (i < num) {
		cond_resched();
		map.m_lblk = i;
		map.m_len = num - i;
		n = ext4_map_blocks(NULL, inode, &map, 0);
		if (n < 0) {
			err = n;
			break;
		}
		if (n == 0) {
			i++;
		} else {
			if (!ext4_data_block_valid_rcu(sbi, system_blks,
						map.m_pblk, n)) {
				err = -EFSCORRUPTED;
				__ext4_error(sb, __func__, __LINE__, -err,
					     map.m_pblk, "blocks %llu-%llu "
					     "from inode %u overlap system zone",
					     map.m_pblk,
					     map.m_pblk + map.m_len - 1, ino);
				break;
			}
			err = add_system_zone(system_blks, map.m_pblk, n);
			if (err < 0)
				break;
			i += n;
		}
	}
	iput(inode);
	return err;
}