static void delete_file(e2fsck_t ctx, ext2_ino_t ino,
			struct dup_inode *dp, char* block_buf)
{
	ext2_filsys fs = ctx->fs;
	struct process_block_struct pb;
	struct problem_context	pctx;
	unsigned int		count;

	clear_problem_context(&pctx);
	pctx.ino = pb.ino = ino;
	pb.dup_blocks = 0;
	pb.ctx = ctx;
	pctx.str = "delete_file";
	pb.cur_cluster = ~0;

	if (ext2fs_inode_has_valid_blocks2(fs, EXT2_INODE(&dp->inode)))
		pctx.errcode = ext2fs_block_iterate3(fs, ino,
						     BLOCK_FLAG_READ_ONLY,
						     block_buf,
						     delete_file_block, &pb);
	if (pctx.errcode)
		fix_problem(ctx, PR_1B_BLOCK_ITERATE, &pctx);
	if (ctx->inode_bad_map)
		ext2fs_unmark_inode_bitmap2(ctx->inode_bad_map, ino);
	ext2fs_inode_alloc_stats2(fs, ino, -1, LINUX_S_ISDIR(dp->inode.i_mode));
	quota_data_sub(ctx->qctx, &dp->inode, ino,
		       pb.dup_blocks * fs->blocksize);
	quota_data_inodes(ctx->qctx, &dp->inode, ino, -1);

	/* Inode may have changed by block_iterate, so reread it */
	e2fsck_read_inode_full(ctx, ino, EXT2_INODE(&dp->inode),
			       sizeof(dp->inode), "delete_file");
	e2fsck_clear_inode(ctx, ino, EXT2_INODE(&dp->inode), 0, "delete_file");
	if (ext2fs_file_acl_block(fs, EXT2_INODE(&dp->inode)) &&
	    ext2fs_has_feature_xattr(fs->super)) {
		blk64_t file_acl_block = ext2fs_file_acl_block(fs,
						EXT2_INODE(&dp->inode));

		count = 1;
		pctx.errcode = ext2fs_adjust_ea_refcount3(fs, file_acl_block,
					block_buf, -1, &count, ino);
		if (pctx.errcode == EXT2_ET_BAD_EA_BLOCK_NUM) {
			pctx.errcode = 0;
			count = 1;
		}
		if (pctx.errcode) {
			pctx.blk = file_acl_block;
			fix_problem(ctx, PR_1B_ADJ_EA_REFCOUNT, &pctx);
		}
		/*
		 * If the count is zero, then arrange to have the
		 * block deleted.  If the block is in the block_dup_map,
		 * also call delete_file_block since it will take care
		 * of keeping the accounting straight.
		 */
		if ((count == 0) ||
		    ext2fs_test_block_bitmap2(ctx->block_dup_map,
					      file_acl_block)) {
			delete_file_block(fs, &file_acl_block,
					  BLOCK_COUNT_EXTATTR, 0, 0, &pb);
			ext2fs_file_acl_block_set(fs, EXT2_INODE(&dp->inode),
						  file_acl_block);
			quota_data_sub(ctx->qctx, &dp->inode, ino,
				       fs->blocksize);
		}
	}
}