xfs_init_new_inode(
	struct xfs_trans	*tp,
	struct xfs_inode	*pip,
	xfs_ino_t		ino,
	umode_t			mode,
	xfs_nlink_t		nlink,
	dev_t			rdev,
	prid_t			prid,
	struct xfs_inode	**ipp)
{
	struct xfs_mount	*mp = tp->t_mountp;
	struct xfs_inode	*ip;
	unsigned int		flags;
	int			error;
	struct timespec64	tv;
	struct inode		*inode;

	/*
	 * Protect against obviously corrupt allocation btree records. Later
	 * xfs_iget checks will catch re-allocation of other active in-memory
	 * and on-disk inodes. If we don't catch reallocating the parent inode
	 * here we will deadlock in xfs_iget() so we have to do these checks
	 * first.
	 */
	if ((pip && ino == pip->i_ino) || !xfs_verify_dir_ino(mp, ino)) {
		xfs_alert(mp, "Allocated a known in-use inode 0x%llx!", ino);
		return -EFSCORRUPTED;
	}

	/*
	 * Get the in-core inode with the lock held exclusively to prevent
	 * others from looking at until we're done.
	 */
	error = xfs_iget(mp, tp, ino, XFS_IGET_CREATE, XFS_ILOCK_EXCL, &ip);
	if (error)
		return error;

	ASSERT(ip != NULL);
	inode = VFS_I(ip);
	inode->i_mode = mode;
	set_nlink(inode, nlink);
	inode->i_uid = current_fsuid();
	inode->i_rdev = rdev;
	ip->i_d.di_projid = prid;

	if (pip && XFS_INHERIT_GID(pip)) {
		inode->i_gid = VFS_I(pip)->i_gid;
		if ((VFS_I(pip)->i_mode & S_ISGID) && S_ISDIR(mode))
			inode->i_mode |= S_ISGID;
	} else {
		inode->i_gid = current_fsgid();
	}

	/*
	 * If the group ID of the new file does not match the effective group
	 * ID or one of the supplementary group IDs, the S_ISGID bit is cleared
	 * (and only if the irix_sgid_inherit compatibility variable is set).
	 */
	if (irix_sgid_inherit &&
	    (inode->i_mode & S_ISGID) && !in_group_p(inode->i_gid))
		inode->i_mode &= ~S_ISGID;

	ip->i_d.di_size = 0;
	ip->i_df.if_nextents = 0;
	ASSERT(ip->i_d.di_nblocks == 0);

	tv = current_time(inode);
	inode->i_mtime = tv;
	inode->i_atime = tv;
	inode->i_ctime = tv;

	ip->i_d.di_extsize = 0;
	ip->i_d.di_dmevmask = 0;
	ip->i_d.di_dmstate = 0;
	ip->i_d.di_flags = 0;

	if (xfs_sb_version_has_v3inode(&mp->m_sb)) {
		inode_set_iversion(inode, 1);
		ip->i_d.di_flags2 = mp->m_ino_geo.new_diflags2;
		ip->i_d.di_cowextsize = 0;
		ip->i_d.di_crtime = tv;
	}

	flags = XFS_ILOG_CORE;
	switch (mode & S_IFMT) {
	case S_IFIFO:
	case S_IFCHR:
	case S_IFBLK:
	case S_IFSOCK:
		ip->i_df.if_format = XFS_DINODE_FMT_DEV;
		ip->i_df.if_flags = 0;
		flags |= XFS_ILOG_DEV;
		break;
	case S_IFREG:
	case S_IFDIR:
		if (pip && (pip->i_d.di_flags & XFS_DIFLAG_ANY))
			xfs_inode_inherit_flags(ip, pip);
		if (pip && (pip->i_d.di_flags2 & XFS_DIFLAG2_ANY))
			xfs_inode_inherit_flags2(ip, pip);
		/* FALLTHROUGH */
	case S_IFLNK:
		ip->i_df.if_format = XFS_DINODE_FMT_EXTENTS;
		ip->i_df.if_flags = XFS_IFEXTENTS;
		ip->i_df.if_bytes = 0;
		ip->i_df.if_u1.if_root = NULL;
		break;
	default:
		ASSERT(0);
	}

	/*
	 * Log the new values stuffed into the inode.
	 */
	xfs_trans_ijoin(tp, ip, XFS_ILOCK_EXCL);
	xfs_trans_log_inode(tp, ip, flags);

	/* now that we have an i_mode we can setup the inode structure */
	xfs_setup_inode(ip);

	*ipp = ip;
	return 0;
}