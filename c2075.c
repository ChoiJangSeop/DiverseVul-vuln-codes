PHPAPI void php_stat(const char *filename, php_stat_len filename_length, int type, zval *return_value TSRMLS_DC)
{
	zval *stat_dev, *stat_ino, *stat_mode, *stat_nlink, *stat_uid, *stat_gid, *stat_rdev,
		 *stat_size, *stat_atime, *stat_mtime, *stat_ctime, *stat_blksize, *stat_blocks;
	struct stat *stat_sb;
	php_stream_statbuf ssb;
	int flags = 0, rmask=S_IROTH, wmask=S_IWOTH, xmask=S_IXOTH; /* access rights defaults to other */
	char *stat_sb_names[13] = {
		"dev", "ino", "mode", "nlink", "uid", "gid", "rdev",
		"size", "atime", "mtime", "ctime", "blksize", "blocks"
	};
	char *local;
	php_stream_wrapper *wrapper;
	char safe_mode_buf[MAXPATHLEN];

	if (!filename_length) {
		RETURN_FALSE;
	}

	if ((wrapper = php_stream_locate_url_wrapper(filename, &local, 0 TSRMLS_CC)) == &php_plain_files_wrapper) {
		if (php_check_open_basedir(local TSRMLS_CC)) {
			RETURN_FALSE;
		} else if (PG(safe_mode)) {
			if (type == FS_IS_X) {
				if (strstr(local, "..")) {
					RETURN_FALSE;
				} else {
					char *b = strrchr(local, PHP_DIR_SEPARATOR);
					snprintf(safe_mode_buf, MAXPATHLEN, "%s%s%s", PG(safe_mode_exec_dir), (b ? "" : "/"), (b ? b : local));
					local = (char *)&safe_mode_buf;
				}
			} else if (!php_checkuid_ex(local, NULL, CHECKUID_ALLOW_FILE_NOT_EXISTS, CHECKUID_NO_ERRORS)) {
				RETURN_FALSE;
			}
		}
	}

	if (IS_ACCESS_CHECK(type)) {
		if (wrapper == &php_plain_files_wrapper) {

			switch (type) {
#ifdef F_OK
				case FS_EXISTS:
					RETURN_BOOL(VCWD_ACCESS(local, F_OK) == 0);
					break;
#endif
#ifdef W_OK
				case FS_IS_W:
					RETURN_BOOL(VCWD_ACCESS(local, W_OK) == 0);
					break;
#endif
#ifdef R_OK
				case FS_IS_R:
					RETURN_BOOL(VCWD_ACCESS(local, R_OK) == 0);
					break;
#endif
#ifdef X_OK
				case FS_IS_X:
					RETURN_BOOL(VCWD_ACCESS(local, X_OK) == 0);
					break;
#endif
			}
		}
	}

	if (IS_LINK_OPERATION(type)) {
		flags |= PHP_STREAM_URL_STAT_LINK;
	}
	if (IS_EXISTS_CHECK(type)) {
		flags |= PHP_STREAM_URL_STAT_QUIET;
	}

	if (php_stream_stat_path_ex((char *)filename, flags, &ssb, NULL)) {
		/* Error Occured */
		if (!IS_EXISTS_CHECK(type)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%sstat failed for %s", IS_LINK_OPERATION(type) ? "L" : "", filename);
		}
		RETURN_FALSE;
	}

	stat_sb = &ssb.sb;


#ifndef NETWARE
	if (type >= FS_IS_W && type <= FS_IS_X) {
		if(ssb.sb.st_uid==getuid()) {
			rmask=S_IRUSR;
			wmask=S_IWUSR;
			xmask=S_IXUSR;
		} else if(ssb.sb.st_gid==getgid()) {
			rmask=S_IRGRP;
			wmask=S_IWGRP;
			xmask=S_IXGRP;
		} else {
			int   groups, n, i;
			gid_t *gids;

			groups = getgroups(0, NULL);
			if(groups > 0) {
				gids=(gid_t *)safe_emalloc(groups, sizeof(gid_t), 0);
				n=getgroups(groups, gids);
				for(i=0;i<n;i++){
					if(ssb.sb.st_gid==gids[i]) {
						rmask=S_IRGRP;
						wmask=S_IWGRP;
						xmask=S_IXGRP;
						break;
					}
				}
				efree(gids);
			}
		}
	}
#endif

#ifndef NETWARE
	if (IS_ABLE_CHECK(type) && getuid() == 0) {
		/* root has special perms on plain_wrapper
		   But we don't know about root under Netware */
		if (wrapper == &php_plain_files_wrapper) {
			if (type == FS_IS_X) {
				xmask = S_IXROOT;
			} else {
				RETURN_TRUE;
			}
		}
	}
#endif

	switch (type) {
	case FS_PERMS:
		RETURN_LONG((long)ssb.sb.st_mode);
	case FS_INODE:
		RETURN_LONG((long)ssb.sb.st_ino);
	case FS_SIZE:
		RETURN_LONG((long)ssb.sb.st_size);
	case FS_OWNER:
		RETURN_LONG((long)ssb.sb.st_uid);
	case FS_GROUP:
		RETURN_LONG((long)ssb.sb.st_gid);
	case FS_ATIME:
		RETURN_LONG((long)ssb.sb.st_atime);
	case FS_MTIME:
		RETURN_LONG((long)ssb.sb.st_mtime);
	case FS_CTIME:
		RETURN_LONG((long)ssb.sb.st_ctime);
	case FS_TYPE:
		if (S_ISLNK(ssb.sb.st_mode)) {
			RETURN_STRING("link", 1);
		}
		switch(ssb.sb.st_mode & S_IFMT) {
		case S_IFIFO: RETURN_STRING("fifo", 1);
		case S_IFCHR: RETURN_STRING("char", 1);
		case S_IFDIR: RETURN_STRING("dir", 1);
		case S_IFBLK: RETURN_STRING("block", 1);
		case S_IFREG: RETURN_STRING("file", 1);
#if defined(S_IFSOCK) && !defined(ZEND_WIN32)&&!defined(__BEOS__)
		case S_IFSOCK: RETURN_STRING("socket", 1);
#endif
		}
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unknown file type (%d)", ssb.sb.st_mode&S_IFMT);
		RETURN_STRING("unknown", 1);
	case FS_IS_W:
		RETURN_BOOL((ssb.sb.st_mode & wmask) != 0);
	case FS_IS_R:
		RETURN_BOOL((ssb.sb.st_mode&rmask)!=0);
	case FS_IS_X:
		RETURN_BOOL((ssb.sb.st_mode&xmask)!=0 && !S_ISDIR(ssb.sb.st_mode));
	case FS_IS_FILE:
		RETURN_BOOL(S_ISREG(ssb.sb.st_mode));
	case FS_IS_DIR:
		RETURN_BOOL(S_ISDIR(ssb.sb.st_mode));
	case FS_IS_LINK:
		RETURN_BOOL(S_ISLNK(ssb.sb.st_mode));
	case FS_EXISTS:
		RETURN_TRUE; /* the false case was done earlier */
	case FS_LSTAT:
		/* FALLTHROUGH */
	case FS_STAT:
		array_init(return_value);

		MAKE_LONG_ZVAL_INCREF(stat_dev, stat_sb->st_dev);
		MAKE_LONG_ZVAL_INCREF(stat_ino, stat_sb->st_ino);
		MAKE_LONG_ZVAL_INCREF(stat_mode, stat_sb->st_mode);
		MAKE_LONG_ZVAL_INCREF(stat_nlink, stat_sb->st_nlink);
		MAKE_LONG_ZVAL_INCREF(stat_uid, stat_sb->st_uid);
		MAKE_LONG_ZVAL_INCREF(stat_gid, stat_sb->st_gid);
#ifdef HAVE_ST_RDEV
		MAKE_LONG_ZVAL_INCREF(stat_rdev, stat_sb->st_rdev);
#else
		MAKE_LONG_ZVAL_INCREF(stat_rdev, -1);
#endif
		MAKE_LONG_ZVAL_INCREF(stat_size, stat_sb->st_size);
		MAKE_LONG_ZVAL_INCREF(stat_atime, stat_sb->st_atime);
		MAKE_LONG_ZVAL_INCREF(stat_mtime, stat_sb->st_mtime);
		MAKE_LONG_ZVAL_INCREF(stat_ctime, stat_sb->st_ctime);
#ifdef HAVE_ST_BLKSIZE
		MAKE_LONG_ZVAL_INCREF(stat_blksize, stat_sb->st_blksize);
#else
		MAKE_LONG_ZVAL_INCREF(stat_blksize,-1);
#endif
#ifdef HAVE_ST_BLOCKS
		MAKE_LONG_ZVAL_INCREF(stat_blocks, stat_sb->st_blocks);
#else
		MAKE_LONG_ZVAL_INCREF(stat_blocks,-1);
#endif
		/* Store numeric indexes in propper order */
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_dev, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_ino, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_mode, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_nlink, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_uid, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_gid, sizeof(zval *), NULL);

		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_rdev, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_size, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_atime, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_mtime, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_ctime, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_blksize, sizeof(zval *), NULL);
		zend_hash_next_index_insert(HASH_OF(return_value), (void *)&stat_blocks, sizeof(zval *), NULL);

		/* Store string indexes referencing the same zval*/
		zend_hash_update(HASH_OF(return_value), stat_sb_names[0], strlen(stat_sb_names[0])+1, (void *) &stat_dev, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[1], strlen(stat_sb_names[1])+1, (void *) &stat_ino, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[2], strlen(stat_sb_names[2])+1, (void *) &stat_mode, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[3], strlen(stat_sb_names[3])+1, (void *) &stat_nlink, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[4], strlen(stat_sb_names[4])+1, (void *) &stat_uid, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[5], strlen(stat_sb_names[5])+1, (void *) &stat_gid, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[6], strlen(stat_sb_names[6])+1, (void *) &stat_rdev, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[7], strlen(stat_sb_names[7])+1, (void *) &stat_size, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[8], strlen(stat_sb_names[8])+1, (void *) &stat_atime, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[9], strlen(stat_sb_names[9])+1, (void *) &stat_mtime, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[10], strlen(stat_sb_names[10])+1, (void *) &stat_ctime, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[11], strlen(stat_sb_names[11])+1, (void *) &stat_blksize, sizeof(zval *), NULL);
		zend_hash_update(HASH_OF(return_value), stat_sb_names[12], strlen(stat_sb_names[12])+1, (void *) &stat_blocks, sizeof(zval *), NULL);

		return;
	}
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Didn't understand stat call");
	RETURN_FALSE;
}