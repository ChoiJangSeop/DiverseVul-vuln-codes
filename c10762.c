int read_super_2(squashfs_operations **s_ops, void *s)
{
	 squashfs_super_block_3 *sBlk_3 = s;

	if(sBlk_3->s_magic != SQUASHFS_MAGIC || sBlk_3->s_major != 2 ||
							sBlk_3->s_minor > 1)
		return -1;

	sBlk.s.s_magic = sBlk_3->s_magic;
	sBlk.s.inodes = sBlk_3->inodes;
	sBlk.s.mkfs_time = sBlk_3->mkfs_time;
	sBlk.s.block_size = sBlk_3->block_size;
	sBlk.s.fragments = sBlk_3->fragments;
	sBlk.s.block_log = sBlk_3->block_log;
	sBlk.s.flags = sBlk_3->flags;
	sBlk.s.s_major = sBlk_3->s_major;
	sBlk.s.s_minor = sBlk_3->s_minor;
	sBlk.s.root_inode = sBlk_3->root_inode;
	sBlk.s.bytes_used = sBlk_3->bytes_used_2;
	sBlk.s.inode_table_start = sBlk_3->inode_table_start;
	sBlk.s.directory_table_start = sBlk_3->directory_table_start_2;
	sBlk.s.fragment_table_start = sBlk_3->fragment_table_start_2;
	sBlk.s.inode_table_start = sBlk_3->inode_table_start_2;
	sBlk.no_uids = sBlk_3->no_uids;
	sBlk.no_guids = sBlk_3->no_guids;
	sBlk.uid_start = sBlk_3->uid_start_2;
	sBlk.guid_start = sBlk_3->guid_start_2;
	sBlk.s.xattr_id_table_start = SQUASHFS_INVALID_BLK;

	*s_ops = &ops;

	/*
	 * 2.x filesystems use gzip compression.
	 */
	comp = lookup_compressor("gzip");
	return TRUE;
}