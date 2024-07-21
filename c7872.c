xfs_fs_nr_cached_objects(
	struct super_block	*sb,
	struct shrink_control	*sc)
{
	return xfs_reclaim_inodes_count(XFS_M(sb));
}