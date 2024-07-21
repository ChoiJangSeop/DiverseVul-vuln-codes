int sqfs_readdir(struct fs_dir_stream *fs_dirs, struct fs_dirent **dentp)
{
	struct squashfs_super_block *sblk = ctxt.sblk;
	struct squashfs_dir_stream *dirs;
	struct squashfs_lreg_inode *lreg;
	struct squashfs_base_inode *base;
	struct squashfs_reg_inode *reg;
	int i_number, offset = 0, ret;
	struct fs_dirent *dent;
	unsigned char *ipos;

	dirs = (struct squashfs_dir_stream *)fs_dirs;
	if (!dirs->size) {
		*dentp = NULL;
		return -SQFS_STOP_READDIR;
	}

	dent = &dirs->dentp;

	if (!dirs->entry_count) {
		if (dirs->size > SQFS_DIR_HEADER_SIZE) {
			dirs->size -= SQFS_DIR_HEADER_SIZE;
		} else {
			*dentp = NULL;
			dirs->size = 0;
			return -SQFS_STOP_READDIR;
		}

		if (dirs->size > SQFS_EMPTY_FILE_SIZE) {
			/* Read follow-up (emitted) dir. header */
			memcpy(dirs->dir_header, dirs->table,
			       SQFS_DIR_HEADER_SIZE);
			dirs->entry_count = dirs->dir_header->count + 1;
			ret = sqfs_read_entry(&dirs->entry, dirs->table +
					      SQFS_DIR_HEADER_SIZE);
			if (ret)
				return -SQFS_STOP_READDIR;

			dirs->table += SQFS_DIR_HEADER_SIZE;
		}
	} else {
		ret = sqfs_read_entry(&dirs->entry, dirs->table);
		if (ret)
			return -SQFS_STOP_READDIR;
	}

	i_number = dirs->dir_header->inode_number + dirs->entry->inode_offset;
	ipos = sqfs_find_inode(dirs->inode_table, i_number, sblk->inodes,
			       sblk->block_size);

	base = (struct squashfs_base_inode *)ipos;

	/* Set entry type and size */
	switch (dirs->entry->type) {
	case SQFS_DIR_TYPE:
	case SQFS_LDIR_TYPE:
		dent->type = FS_DT_DIR;
		break;
	case SQFS_REG_TYPE:
	case SQFS_LREG_TYPE:
		/*
		 * Entries do not differentiate extended from regular types, so
		 * it needs to be verified manually.
		 */
		if (get_unaligned_le16(&base->inode_type) == SQFS_LREG_TYPE) {
			lreg = (struct squashfs_lreg_inode *)ipos;
			dent->size = get_unaligned_le64(&lreg->file_size);
		} else {
			reg = (struct squashfs_reg_inode *)ipos;
			dent->size = get_unaligned_le32(&reg->file_size);
		}

		dent->type = FS_DT_REG;
		break;
	case SQFS_BLKDEV_TYPE:
	case SQFS_CHRDEV_TYPE:
	case SQFS_LBLKDEV_TYPE:
	case SQFS_LCHRDEV_TYPE:
	case SQFS_FIFO_TYPE:
	case SQFS_SOCKET_TYPE:
	case SQFS_LFIFO_TYPE:
	case SQFS_LSOCKET_TYPE:
		dent->type = SQFS_MISC_ENTRY_TYPE;
		break;
	case SQFS_SYMLINK_TYPE:
	case SQFS_LSYMLINK_TYPE:
		dent->type = FS_DT_LNK;
		break;
	default:
		return -SQFS_STOP_READDIR;
	}

	/* Set entry name */
	strncpy(dent->name, dirs->entry->name, dirs->entry->name_size + 1);
	dent->name[dirs->entry->name_size + 1] = '\0';

	offset = dirs->entry->name_size + 1 + SQFS_ENTRY_BASE_LENGTH;
	dirs->entry_count--;

	/* Decrement size to be read */
	if (dirs->size > offset)
		dirs->size -= offset;
	else
		dirs->size = 0;

	/* Keep a reference to the current entry before incrementing it */
	dirs->table += offset;

	*dentp = dent;

	return 0;
}