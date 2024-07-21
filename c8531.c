static int fill_dir_block(ext2_filsys fs,
			  blk64_t *block_nr,
			  e2_blkcnt_t blockcnt,
			  blk64_t ref_block EXT2FS_ATTR((unused)),
			  int ref_offset EXT2FS_ATTR((unused)),
			  void *priv_data)
{
	struct fill_dir_struct	*fd = (struct fill_dir_struct *) priv_data;
	struct hash_entry 	*new_array, *ent;
	struct ext2_dir_entry 	*dirent;
	char			*dir;
	unsigned int		offset, dir_offset, rec_len, name_len;
	int			hash_alg, hash_flags;

	if (blockcnt < 0)
		return 0;

	offset = blockcnt * fs->blocksize;
	if (offset + fs->blocksize > fd->inode->i_size) {
		fd->err = EXT2_ET_DIR_CORRUPTED;
		return BLOCK_ABORT;
	}

	dir = (fd->buf+offset);
	if (*block_nr == 0) {
		memset(dir, 0, fs->blocksize);
		dirent = (struct ext2_dir_entry *) dir;
		(void) ext2fs_set_rec_len(fs, fs->blocksize, dirent);
	} else {
		int flags = fs->flags;
		fs->flags |= EXT2_FLAG_IGNORE_CSUM_ERRORS;
		fd->err = ext2fs_read_dir_block4(fs, *block_nr, dir, 0,
						 fd->dir);
		fs->flags = (flags & EXT2_FLAG_IGNORE_CSUM_ERRORS) |
			    (fs->flags & ~EXT2_FLAG_IGNORE_CSUM_ERRORS);
		if (fd->err)
			return BLOCK_ABORT;
	}
	hash_flags = fd->inode->i_flags & EXT4_CASEFOLD_FL;
	hash_alg = fs->super->s_def_hash_version;
	if ((hash_alg <= EXT2_HASH_TEA) &&
	    (fs->super->s_flags & EXT2_FLAGS_UNSIGNED_HASH))
		hash_alg += 3;
	/* While the directory block is "hot", index it. */
	dir_offset = 0;
	while (dir_offset < fs->blocksize) {
		dirent = (struct ext2_dir_entry *) (dir + dir_offset);
		(void) ext2fs_get_rec_len(fs, dirent, &rec_len);
		name_len = ext2fs_dirent_name_len(dirent);
		if (((dir_offset + rec_len) > fs->blocksize) ||
		    (rec_len < 8) ||
		    ((rec_len % 4) != 0) ||
		    (name_len + 8 > rec_len)) {
			fd->err = EXT2_ET_DIR_CORRUPTED;
			return BLOCK_ABORT;
		}
		dir_offset += rec_len;
		if (dirent->inode == 0)
			continue;
		if (!fd->compress && (name_len == 1) &&
		    (dirent->name[0] == '.'))
			continue;
		if (!fd->compress && (name_len == 2) &&
		    (dirent->name[0] == '.') && (dirent->name[1] == '.')) {
			fd->parent = dirent->inode;
			continue;
		}
		if (fd->num_array >= fd->max_array) {
			new_array = realloc(fd->harray,
			    sizeof(struct hash_entry) * (fd->max_array+500));
			if (!new_array) {
				fd->err = ENOMEM;
				return BLOCK_ABORT;
			}
			fd->harray = new_array;
			fd->max_array += 500;
		}
		ent = fd->harray + fd->num_array++;
		ent->dir = dirent;
		fd->dir_size += EXT2_DIR_REC_LEN(name_len);
		ent->ino = dirent->inode;
		if (fd->compress)
			ent->hash = ent->minor_hash = 0;
		else {
			fd->err = ext2fs_dirhash2(hash_alg,
						  dirent->name, name_len,
						  fs->encoding, hash_flags,
						  fs->super->s_hash_seed,
						  &ent->hash, &ent->minor_hash);
			if (fd->err)
				return BLOCK_ABORT;
		}
	}

	return 0;
}