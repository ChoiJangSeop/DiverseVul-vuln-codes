xfs_attr_leaf_addname(xfs_da_args_t *args)
{
	xfs_inode_t *dp;
	struct xfs_buf *bp;
	int retval, error, committed, forkoff;

	trace_xfs_attr_leaf_addname(args);

	/*
	 * Read the (only) block in the attribute list in.
	 */
	dp = args->dp;
	args->blkno = 0;
	error = xfs_attr3_leaf_read(args->trans, args->dp, args->blkno, -1, &bp);
	if (error)
		return error;

	/*
	 * Look up the given attribute in the leaf block.  Figure out if
	 * the given flags produce an error or call for an atomic rename.
	 */
	retval = xfs_attr3_leaf_lookup_int(bp, args);
	if ((args->flags & ATTR_REPLACE) && (retval == ENOATTR)) {
		xfs_trans_brelse(args->trans, bp);
		return retval;
	} else if (retval == EEXIST) {
		if (args->flags & ATTR_CREATE) {	/* pure create op */
			xfs_trans_brelse(args->trans, bp);
			return retval;
		}

		trace_xfs_attr_leaf_replace(args);

		args->op_flags |= XFS_DA_OP_RENAME;	/* an atomic rename */
		args->blkno2 = args->blkno;		/* set 2nd entry info*/
		args->index2 = args->index;
		args->rmtblkno2 = args->rmtblkno;
		args->rmtblkcnt2 = args->rmtblkcnt;
	}

	/*
	 * Add the attribute to the leaf block, transitioning to a Btree
	 * if required.
	 */
	retval = xfs_attr3_leaf_add(bp, args);
	if (retval == ENOSPC) {
		/*
		 * Promote the attribute list to the Btree format, then
		 * Commit that transaction so that the node_addname() call
		 * can manage its own transactions.
		 */
		xfs_bmap_init(args->flist, args->firstblock);
		error = xfs_attr3_leaf_to_node(args);
		if (!error) {
			error = xfs_bmap_finish(&args->trans, args->flist,
						&committed);
		}
		if (error) {
			ASSERT(committed);
			args->trans = NULL;
			xfs_bmap_cancel(args->flist);
			return(error);
		}

		/*
		 * bmap_finish() may have committed the last trans and started
		 * a new one.  We need the inode to be in all transactions.
		 */
		if (committed)
			xfs_trans_ijoin(args->trans, dp, 0);

		/*
		 * Commit the current trans (including the inode) and start
		 * a new one.
		 */
		error = xfs_trans_roll(&args->trans, dp);
		if (error)
			return (error);

		/*
		 * Fob the whole rest of the problem off on the Btree code.
		 */
		error = xfs_attr_node_addname(args);
		return(error);
	}

	/*
	 * Commit the transaction that added the attr name so that
	 * later routines can manage their own transactions.
	 */
	error = xfs_trans_roll(&args->trans, dp);
	if (error)
		return (error);

	/*
	 * If there was an out-of-line value, allocate the blocks we
	 * identified for its storage and copy the value.  This is done
	 * after we create the attribute so that we don't overflow the
	 * maximum size of a transaction and/or hit a deadlock.
	 */
	if (args->rmtblkno > 0) {
		error = xfs_attr_rmtval_set(args);
		if (error)
			return(error);
	}

	/*
	 * If this is an atomic rename operation, we must "flip" the
	 * incomplete flags on the "new" and "old" attribute/value pairs
	 * so that one disappears and one appears atomically.  Then we
	 * must remove the "old" attribute/value pair.
	 */
	if (args->op_flags & XFS_DA_OP_RENAME) {
		/*
		 * In a separate transaction, set the incomplete flag on the
		 * "old" attr and clear the incomplete flag on the "new" attr.
		 */
		error = xfs_attr3_leaf_flipflags(args);
		if (error)
			return(error);

		/*
		 * Dismantle the "old" attribute/value pair by removing
		 * a "remote" value (if it exists).
		 */
		args->index = args->index2;
		args->blkno = args->blkno2;
		args->rmtblkno = args->rmtblkno2;
		args->rmtblkcnt = args->rmtblkcnt2;
		if (args->rmtblkno) {
			error = xfs_attr_rmtval_remove(args);
			if (error)
				return(error);
		}

		/*
		 * Read in the block containing the "old" attr, then
		 * remove the "old" attr from that block (neat, huh!)
		 */
		error = xfs_attr3_leaf_read(args->trans, args->dp, args->blkno,
					   -1, &bp);
		if (error)
			return error;

		xfs_attr3_leaf_remove(bp, args);

		/*
		 * If the result is small enough, shrink it all into the inode.
		 */
		if ((forkoff = xfs_attr_shortform_allfit(bp, dp))) {
			xfs_bmap_init(args->flist, args->firstblock);
			error = xfs_attr3_leaf_to_shortform(bp, args, forkoff);
			/* bp is gone due to xfs_da_shrink_inode */
			if (!error) {
				error = xfs_bmap_finish(&args->trans,
							args->flist,
							&committed);
			}
			if (error) {
				ASSERT(committed);
				args->trans = NULL;
				xfs_bmap_cancel(args->flist);
				return(error);
			}

			/*
			 * bmap_finish() may have committed the last trans
			 * and started a new one.  We need the inode to be
			 * in all transactions.
			 */
			if (committed)
				xfs_trans_ijoin(args->trans, dp, 0);
		}

		/*
		 * Commit the remove and start the next trans in series.
		 */
		error = xfs_trans_roll(&args->trans, dp);

	} else if (args->rmtblkno > 0) {
		/*
		 * Added a "remote" value, just clear the incomplete flag.
		 */
		error = xfs_attr3_leaf_clearflag(args);
	}
	return error;
}