xfs_file_fsync(
	struct file		*file,
	loff_t			start,
	loff_t			end,
	int			datasync)
{
	struct inode		*inode = file->f_mapping->host;
	struct xfs_inode	*ip = XFS_I(inode);
	struct xfs_mount	*mp = ip->i_mount;
	int			error = 0;
	int			log_flushed = 0;
	xfs_lsn_t		lsn = 0;

	trace_xfs_file_fsync(ip);

	error = filemap_write_and_wait_range(inode->i_mapping, start, end);
	if (error)
		return error;

	if (XFS_FORCED_SHUTDOWN(mp))
		return -EIO;

	xfs_iflags_clear(ip, XFS_ITRUNCATED);

	if (mp->m_flags & XFS_MOUNT_BARRIER) {
		/*
		 * If we have an RT and/or log subvolume we need to make sure
		 * to flush the write cache the device used for file data
		 * first.  This is to ensure newly written file data make
		 * it to disk before logging the new inode size in case of
		 * an extending write.
		 */
		if (XFS_IS_REALTIME_INODE(ip))
			xfs_blkdev_issue_flush(mp->m_rtdev_targp);
		else if (mp->m_logdev_targp != mp->m_ddev_targp)
			xfs_blkdev_issue_flush(mp->m_ddev_targp);
	}

	/*
	 * All metadata updates are logged, which means that we just have
	 * to flush the log up to the latest LSN that touched the inode.
	 */
	xfs_ilock(ip, XFS_ILOCK_SHARED);
	if (xfs_ipincount(ip)) {
		if (!datasync ||
		    (ip->i_itemp->ili_fields & ~XFS_ILOG_TIMESTAMP))
			lsn = ip->i_itemp->ili_last_lsn;
	}
	xfs_iunlock(ip, XFS_ILOCK_SHARED);

	if (lsn)
		error = _xfs_log_force_lsn(mp, lsn, XFS_LOG_SYNC, &log_flushed);

	/*
	 * If we only have a single device, and the log force about was
	 * a no-op we might have to flush the data device cache here.
	 * This can only happen for fdatasync/O_DSYNC if we were overwriting
	 * an already allocated file and thus do not have any metadata to
	 * commit.
	 */
	if ((mp->m_flags & XFS_MOUNT_BARRIER) &&
	    mp->m_logdev_targp == mp->m_ddev_targp &&
	    !XFS_IS_REALTIME_INODE(ip) &&
	    !log_flushed)
		xfs_blkdev_issue_flush(mp->m_ddev_targp);

	return error;
}