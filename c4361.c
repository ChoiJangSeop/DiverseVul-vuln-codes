xfs_trans_log_inode(
	xfs_trans_t	*tp,
	xfs_inode_t	*ip,
	uint		flags)
{
	ASSERT(ip->i_itemp != NULL);
	ASSERT(xfs_isilocked(ip, XFS_ILOCK_EXCL));

	/*
	 * First time we log the inode in a transaction, bump the inode change
	 * counter if it is configured for this to occur. We don't use
	 * inode_inc_version() because there is no need for extra locking around
	 * i_version as we already hold the inode locked exclusively for
	 * metadata modification.
	 */
	if (!(ip->i_itemp->ili_item.li_desc->lid_flags & XFS_LID_DIRTY) &&
	    IS_I_VERSION(VFS_I(ip))) {
		ip->i_d.di_changecount = ++VFS_I(ip)->i_version;
		flags |= XFS_ILOG_CORE;
	}

	tp->t_flags |= XFS_TRANS_DIRTY;
	ip->i_itemp->ili_item.li_desc->lid_flags |= XFS_LID_DIRTY;

	/*
	 * Always OR in the bits from the ili_last_fields field.
	 * This is to coordinate with the xfs_iflush() and xfs_iflush_done()
	 * routines in the eventual clearing of the ili_fields bits.
	 * See the big comment in xfs_iflush() for an explanation of
	 * this coordination mechanism.
	 */
	flags |= ip->i_itemp->ili_last_fields;
	ip->i_itemp->ili_fields |= flags;
}