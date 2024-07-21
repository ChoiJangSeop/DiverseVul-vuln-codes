static int duplicate_search_and_fix(e2fsck_t ctx, ext2_filsys fs,
				    ext2_ino_t ino,
				    struct fill_dir_struct *fd)
{
	struct problem_context	pctx;
	struct hash_entry 	*ent, *prev;
	int			i, j;
	int			fixed = 0;
	char			new_name[256];
	unsigned int		new_len;
	int			hash_alg;
	int hash_flags = fd->inode->i_flags & EXT4_CASEFOLD_FL;

	clear_problem_context(&pctx);
	pctx.ino = ino;

	hash_alg = fs->super->s_def_hash_version;
	if ((hash_alg <= EXT2_HASH_TEA) &&
	    (fs->super->s_flags & EXT2_FLAGS_UNSIGNED_HASH))
		hash_alg += 3;

	for (i=1; i < fd->num_array; i++) {
		ent = fd->harray + i;
		prev = ent - 1;
		if (!ent->dir->inode ||
		    (ext2fs_dirent_name_len(ent->dir) !=
		     ext2fs_dirent_name_len(prev->dir)) ||
		    memcmp(ent->dir->name, prev->dir->name,
			     ext2fs_dirent_name_len(ent->dir)))
			continue;
		pctx.dirent = ent->dir;
		if ((ent->dir->inode == prev->dir->inode) &&
		    fix_problem(ctx, PR_2_DUPLICATE_DIRENT, &pctx)) {
			e2fsck_adjust_inode_count(ctx, ent->dir->inode, -1);
			ent->dir->inode = 0;
			fixed++;
			continue;
		}
		new_len = ext2fs_dirent_name_len(ent->dir);
		memcpy(new_name, ent->dir->name, new_len);
		mutate_name(new_name, &new_len);
		for (j=0; j < fd->num_array; j++) {
			if ((i==j) ||
			    (new_len !=
			     (unsigned) ext2fs_dirent_name_len(fd->harray[j].dir)) ||
			    memcmp(new_name, fd->harray[j].dir->name, new_len))
				continue;
			mutate_name(new_name, &new_len);

			j = -1;
		}
		new_name[new_len] = 0;
		pctx.str = new_name;
		if (fix_problem(ctx, PR_2_NON_UNIQUE_FILE, &pctx)) {
			memcpy(ent->dir->name, new_name, new_len);
			ext2fs_dirent_set_name_len(ent->dir, new_len);
			ext2fs_dirhash2(hash_alg, new_name, new_len,
					fs->encoding, hash_flags,
					fs->super->s_hash_seed,
					&ent->hash, &ent->minor_hash);
			fixed++;
		}
	}
	return fixed;
}