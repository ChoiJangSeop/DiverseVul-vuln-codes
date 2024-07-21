static int __check_block_validity(struct inode *inode, const char *func,
				unsigned int line,
				struct ext4_map_blocks *map)
{
	if (ext4_has_feature_journal(inode->i_sb) &&
	    (inode->i_ino ==
	     le32_to_cpu(EXT4_SB(inode->i_sb)->s_es->s_journal_inum)))
		return 0;
	if (!ext4_data_block_valid(EXT4_SB(inode->i_sb), map->m_pblk,
				   map->m_len)) {
		ext4_error_inode(inode, func, line, map->m_pblk,
				 "lblock %lu mapped to illegal pblock %llu "
				 "(length %d)", (unsigned long) map->m_lblk,
				 map->m_pblk, map->m_len);
		return -EFSCORRUPTED;
	}
	return 0;
}