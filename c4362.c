xfs_ifree_cluster(
	xfs_inode_t		*free_ip,
	xfs_trans_t		*tp,
	struct xfs_icluster	*xic)
{
	xfs_mount_t		*mp = free_ip->i_mount;
	int			blks_per_cluster;
	int			inodes_per_cluster;
	int			nbufs;
	int			i, j;
	int			ioffset;
	xfs_daddr_t		blkno;
	xfs_buf_t		*bp;
	xfs_inode_t		*ip;
	xfs_inode_log_item_t	*iip;
	xfs_log_item_t		*lip;
	struct xfs_perag	*pag;
	xfs_ino_t		inum;

	inum = xic->first_ino;
	pag = xfs_perag_get(mp, XFS_INO_TO_AGNO(mp, inum));
	blks_per_cluster = xfs_icluster_size_fsb(mp);
	inodes_per_cluster = blks_per_cluster << mp->m_sb.sb_inopblog;
	nbufs = mp->m_ialloc_blks / blks_per_cluster;

	for (j = 0; j < nbufs; j++, inum += inodes_per_cluster) {
		/*
		 * The allocation bitmap tells us which inodes of the chunk were
		 * physically allocated. Skip the cluster if an inode falls into
		 * a sparse region.
		 */
		ioffset = inum - xic->first_ino;
		if ((xic->alloc & XFS_INOBT_MASK(ioffset)) == 0) {
			ASSERT(do_mod(ioffset, inodes_per_cluster) == 0);
			continue;
		}

		blkno = XFS_AGB_TO_DADDR(mp, XFS_INO_TO_AGNO(mp, inum),
					 XFS_INO_TO_AGBNO(mp, inum));

		/*
		 * We obtain and lock the backing buffer first in the process
		 * here, as we have to ensure that any dirty inode that we
		 * can't get the flush lock on is attached to the buffer.
		 * If we scan the in-memory inodes first, then buffer IO can
		 * complete before we get a lock on it, and hence we may fail
		 * to mark all the active inodes on the buffer stale.
		 */
		bp = xfs_trans_get_buf(tp, mp->m_ddev_targp, blkno,
					mp->m_bsize * blks_per_cluster,
					XBF_UNMAPPED);

		if (!bp)
			return -ENOMEM;

		/*
		 * This buffer may not have been correctly initialised as we
		 * didn't read it from disk. That's not important because we are
		 * only using to mark the buffer as stale in the log, and to
		 * attach stale cached inodes on it. That means it will never be
		 * dispatched for IO. If it is, we want to know about it, and we
		 * want it to fail. We can acheive this by adding a write
		 * verifier to the buffer.
		 */
		 bp->b_ops = &xfs_inode_buf_ops;

		/*
		 * Walk the inodes already attached to the buffer and mark them
		 * stale. These will all have the flush locks held, so an
		 * in-memory inode walk can't lock them. By marking them all
		 * stale first, we will not attempt to lock them in the loop
		 * below as the XFS_ISTALE flag will be set.
		 */
		lip = bp->b_fspriv;
		while (lip) {
			if (lip->li_type == XFS_LI_INODE) {
				iip = (xfs_inode_log_item_t *)lip;
				ASSERT(iip->ili_logged == 1);
				lip->li_cb = xfs_istale_done;
				xfs_trans_ail_copy_lsn(mp->m_ail,
							&iip->ili_flush_lsn,
							&iip->ili_item.li_lsn);
				xfs_iflags_set(iip->ili_inode, XFS_ISTALE);
			}
			lip = lip->li_bio_list;
		}


		/*
		 * For each inode in memory attempt to add it to the inode
		 * buffer and set it up for being staled on buffer IO
		 * completion.  This is safe as we've locked out tail pushing
		 * and flushing by locking the buffer.
		 *
		 * We have already marked every inode that was part of a
		 * transaction stale above, which means there is no point in
		 * even trying to lock them.
		 */
		for (i = 0; i < inodes_per_cluster; i++) {
retry:
			rcu_read_lock();
			ip = radix_tree_lookup(&pag->pag_ici_root,
					XFS_INO_TO_AGINO(mp, (inum + i)));

			/* Inode not in memory, nothing to do */
			if (!ip) {
				rcu_read_unlock();
				continue;
			}

			/*
			 * because this is an RCU protected lookup, we could
			 * find a recently freed or even reallocated inode
			 * during the lookup. We need to check under the
			 * i_flags_lock for a valid inode here. Skip it if it
			 * is not valid, the wrong inode or stale.
			 */
			spin_lock(&ip->i_flags_lock);
			if (ip->i_ino != inum + i ||
			    __xfs_iflags_test(ip, XFS_ISTALE)) {
				spin_unlock(&ip->i_flags_lock);
				rcu_read_unlock();
				continue;
			}
			spin_unlock(&ip->i_flags_lock);

			/*
			 * Don't try to lock/unlock the current inode, but we
			 * _cannot_ skip the other inodes that we did not find
			 * in the list attached to the buffer and are not
			 * already marked stale. If we can't lock it, back off
			 * and retry.
			 */
			if (ip != free_ip &&
			    !xfs_ilock_nowait(ip, XFS_ILOCK_EXCL)) {
				rcu_read_unlock();
				delay(1);
				goto retry;
			}
			rcu_read_unlock();

			xfs_iflock(ip);
			xfs_iflags_set(ip, XFS_ISTALE);

			/*
			 * we don't need to attach clean inodes or those only
			 * with unlogged changes (which we throw away, anyway).
			 */
			iip = ip->i_itemp;
			if (!iip || xfs_inode_clean(ip)) {
				ASSERT(ip != free_ip);
				xfs_ifunlock(ip);
				xfs_iunlock(ip, XFS_ILOCK_EXCL);
				continue;
			}

			iip->ili_last_fields = iip->ili_fields;
			iip->ili_fields = 0;
			iip->ili_logged = 1;
			xfs_trans_ail_copy_lsn(mp->m_ail, &iip->ili_flush_lsn,
						&iip->ili_item.li_lsn);

			xfs_buf_attach_iodone(bp, xfs_istale_done,
						  &iip->ili_item);

			if (ip != free_ip)
				xfs_iunlock(ip, XFS_ILOCK_EXCL);
		}

		xfs_trans_stale_inode_buf(tp, bp);
		xfs_trans_binval(tp, bp);
	}

	xfs_perag_put(pag);
	return 0;
}