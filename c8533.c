void e2fsck_rehash_directories(e2fsck_t ctx)
{
	struct problem_context	pctx;
#ifdef RESOURCE_TRACK
	struct resource_track	rtrack;
#endif
	struct dir_info		*dir;
	ext2_u32_iterate 	iter;
	struct dir_info_iter *	dirinfo_iter = 0;
	ext2_ino_t		ino;
	errcode_t		retval;
	int			cur, max, all_dirs, first = 1;

	init_resource_track(&rtrack, ctx->fs->io);
	all_dirs = ctx->options & E2F_OPT_COMPRESS_DIRS;

	if (!ctx->dirs_to_hash && !all_dirs)
		return;

	(void) e2fsck_get_lost_and_found(ctx, 0);

	clear_problem_context(&pctx);

	cur = 0;
	if (all_dirs) {
		dirinfo_iter = e2fsck_dir_info_iter_begin(ctx);
		max = e2fsck_get_num_dirinfo(ctx);
	} else {
		retval = ext2fs_u32_list_iterate_begin(ctx->dirs_to_hash,
						       &iter);
		if (retval) {
			pctx.errcode = retval;
			fix_problem(ctx, PR_3A_OPTIMIZE_ITER, &pctx);
			return;
		}
		max = ext2fs_u32_list_count(ctx->dirs_to_hash);
	}
	while (1) {
		if (all_dirs) {
			if ((dir = e2fsck_dir_info_iter(ctx,
							dirinfo_iter)) == 0)
				break;
			ino = dir->ino;
		} else {
			if (!ext2fs_u32_list_iterate(iter, &ino))
				break;
		}

		pctx.dir = ino;
		if (first) {
			fix_problem(ctx, PR_3A_PASS_HEADER, &pctx);
			first = 0;
		}
#if 0
		fix_problem(ctx, PR_3A_OPTIMIZE_DIR, &pctx);
#endif
		pctx.errcode = e2fsck_rehash_dir(ctx, ino, &pctx);
		if (pctx.errcode) {
			end_problem_latch(ctx, PR_LATCH_OPTIMIZE_DIR);
			fix_problem(ctx, PR_3A_OPTIMIZE_DIR_ERR, &pctx);
		}
		if (ctx->progress && !ctx->progress_fd)
			e2fsck_simple_progress(ctx, "Rebuilding directory",
			       100.0 * (float) (++cur) / (float) max, ino);
	}
	end_problem_latch(ctx, PR_LATCH_OPTIMIZE_DIR);
	if (all_dirs)
		e2fsck_dir_info_iter_end(ctx, dirinfo_iter);
	else
		ext2fs_u32_list_iterate_end(iter);

	if (ctx->dirs_to_hash)
		ext2fs_u32_list_free(ctx->dirs_to_hash);
	ctx->dirs_to_hash = 0;

	print_resource_track(ctx, "Pass 3A", &rtrack, ctx->fs->io);
}