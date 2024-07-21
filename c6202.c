xfs_bmap_extents_to_btree(
	xfs_trans_t		*tp,		/* transaction pointer */
	xfs_inode_t		*ip,		/* incore inode pointer */
	xfs_fsblock_t		*firstblock,	/* first-block-allocated */
	struct xfs_defer_ops	*dfops,		/* blocks freed in xaction */
	xfs_btree_cur_t		**curp,		/* cursor returned to caller */
	int			wasdel,		/* converting a delayed alloc */
	int			*logflagsp,	/* inode logging flags */
	int			whichfork)	/* data or attr fork */
{
	struct xfs_btree_block	*ablock;	/* allocated (child) bt block */
	xfs_buf_t		*abp;		/* buffer for ablock */
	xfs_alloc_arg_t		args;		/* allocation arguments */
	xfs_bmbt_rec_t		*arp;		/* child record pointer */
	struct xfs_btree_block	*block;		/* btree root block */
	xfs_btree_cur_t		*cur;		/* bmap btree cursor */
	int			error;		/* error return value */
	xfs_ifork_t		*ifp;		/* inode fork pointer */
	xfs_bmbt_key_t		*kp;		/* root block key pointer */
	xfs_mount_t		*mp;		/* mount structure */
	xfs_bmbt_ptr_t		*pp;		/* root block address pointer */
	struct xfs_iext_cursor	icur;
	struct xfs_bmbt_irec	rec;
	xfs_extnum_t		cnt = 0;

	mp = ip->i_mount;
	ASSERT(whichfork != XFS_COW_FORK);
	ifp = XFS_IFORK_PTR(ip, whichfork);
	ASSERT(XFS_IFORK_FORMAT(ip, whichfork) == XFS_DINODE_FMT_EXTENTS);

	/*
	 * Make space in the inode incore.
	 */
	xfs_iroot_realloc(ip, 1, whichfork);
	ifp->if_flags |= XFS_IFBROOT;

	/*
	 * Fill in the root.
	 */
	block = ifp->if_broot;
	xfs_btree_init_block_int(mp, block, XFS_BUF_DADDR_NULL,
				 XFS_BTNUM_BMAP, 1, 1, ip->i_ino,
				 XFS_BTREE_LONG_PTRS);
	/*
	 * Need a cursor.  Can't allocate until bb_level is filled in.
	 */
	cur = xfs_bmbt_init_cursor(mp, tp, ip, whichfork);
	cur->bc_private.b.firstblock = *firstblock;
	cur->bc_private.b.dfops = dfops;
	cur->bc_private.b.flags = wasdel ? XFS_BTCUR_BPRV_WASDEL : 0;
	/*
	 * Convert to a btree with two levels, one record in root.
	 */
	XFS_IFORK_FMT_SET(ip, whichfork, XFS_DINODE_FMT_BTREE);
	memset(&args, 0, sizeof(args));
	args.tp = tp;
	args.mp = mp;
	xfs_rmap_ino_bmbt_owner(&args.oinfo, ip->i_ino, whichfork);
	args.firstblock = *firstblock;
	if (*firstblock == NULLFSBLOCK) {
		args.type = XFS_ALLOCTYPE_START_BNO;
		args.fsbno = XFS_INO_TO_FSB(mp, ip->i_ino);
	} else if (dfops->dop_low) {
		args.type = XFS_ALLOCTYPE_START_BNO;
		args.fsbno = *firstblock;
	} else {
		args.type = XFS_ALLOCTYPE_NEAR_BNO;
		args.fsbno = *firstblock;
	}
	args.minlen = args.maxlen = args.prod = 1;
	args.wasdel = wasdel;
	*logflagsp = 0;
	if ((error = xfs_alloc_vextent(&args))) {
		xfs_iroot_realloc(ip, -1, whichfork);
		xfs_btree_del_cursor(cur, XFS_BTREE_ERROR);
		return error;
	}

	if (WARN_ON_ONCE(args.fsbno == NULLFSBLOCK)) {
		xfs_iroot_realloc(ip, -1, whichfork);
		xfs_btree_del_cursor(cur, XFS_BTREE_ERROR);
		return -ENOSPC;
	}
	/*
	 * Allocation can't fail, the space was reserved.
	 */
	ASSERT(*firstblock == NULLFSBLOCK ||
	       args.agno >= XFS_FSB_TO_AGNO(mp, *firstblock));
	*firstblock = cur->bc_private.b.firstblock = args.fsbno;
	cur->bc_private.b.allocated++;
	ip->i_d.di_nblocks++;
	xfs_trans_mod_dquot_byino(tp, ip, XFS_TRANS_DQ_BCOUNT, 1L);
	abp = xfs_btree_get_bufl(mp, tp, args.fsbno, 0);
	/*
	 * Fill in the child block.
	 */
	abp->b_ops = &xfs_bmbt_buf_ops;
	ablock = XFS_BUF_TO_BLOCK(abp);
	xfs_btree_init_block_int(mp, ablock, abp->b_bn,
				XFS_BTNUM_BMAP, 0, 0, ip->i_ino,
				XFS_BTREE_LONG_PTRS);

	for_each_xfs_iext(ifp, &icur, &rec) {
		if (isnullstartblock(rec.br_startblock))
			continue;
		arp = XFS_BMBT_REC_ADDR(mp, ablock, 1 + cnt);
		xfs_bmbt_disk_set_all(arp, &rec);
		cnt++;
	}
	ASSERT(cnt == XFS_IFORK_NEXTENTS(ip, whichfork));
	xfs_btree_set_numrecs(ablock, cnt);

	/*
	 * Fill in the root key and pointer.
	 */
	kp = XFS_BMBT_KEY_ADDR(mp, block, 1);
	arp = XFS_BMBT_REC_ADDR(mp, ablock, 1);
	kp->br_startoff = cpu_to_be64(xfs_bmbt_disk_get_startoff(arp));
	pp = XFS_BMBT_PTR_ADDR(mp, block, 1, xfs_bmbt_get_maxrecs(cur,
						be16_to_cpu(block->bb_level)));
	*pp = cpu_to_be64(args.fsbno);

	/*
	 * Do all this logging at the end so that
	 * the root is at the right level.
	 */
	xfs_btree_log_block(cur, abp, XFS_BB_ALL_BITS);
	xfs_btree_log_recs(cur, abp, 1, be16_to_cpu(ablock->bb_numrecs));
	ASSERT(*curp == NULL);
	*curp = cur;
	*logflagsp = XFS_ILOG_CORE | xfs_ilog_fbroot(whichfork);
	return 0;
}