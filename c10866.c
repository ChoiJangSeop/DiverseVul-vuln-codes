_archive_write_disk_close(struct archive *_a)
{
	struct archive_write_disk *a = (struct archive_write_disk *)_a;
	struct fixup_entry *next, *p;
	struct stat st;
	char *c;
	int fd, ret;

	archive_check_magic(&a->archive, ARCHIVE_WRITE_DISK_MAGIC,
	    ARCHIVE_STATE_HEADER | ARCHIVE_STATE_DATA,
	    "archive_write_disk_close");
	ret = _archive_write_disk_finish_entry(&a->archive);

	/* Sort dir list so directories are fixed up in depth-first order. */
	p = sort_dir_list(a->fixup_list);

	while (p != NULL) {
		fd = -1;
		a->pst = NULL; /* Mark stat cache as out-of-date. */

		/* We must strip trailing slashes from the path to avoid
		   dereferencing symbolic links to directories */
		c = p->name;
		while (*c != '\0')
			c++;
		while (c != p->name && *(c - 1) == '/') {
			c--;
			*c = '\0';
		}

		if (p->fixup == 0)
			goto skip_fixup_entry;
		else {
			fd = open(p->name, O_BINARY | O_NOFOLLOW | O_RDONLY
#if defined(O_DIRECTORY)
			    | O_DIRECTORY
#endif
			    | O_CLOEXEC);
			/*
		 `	 * If we don't support O_DIRECTORY,
			 * or open() has failed, we must stat()
			 * to verify that we are opening a directory
			 */
#if defined(O_DIRECTORY)
			if (fd == -1) {
				if (lstat(p->name, &st) != 0 ||
				    !S_ISDIR(st.st_mode)) {
					goto skip_fixup_entry;
				}
			}
#else
#if HAVE_FSTAT
			if (fd > 0 && (
			    fstat(fd, &st) != 0 || !S_ISDIR(st.st_mode))) {
				goto skip_fixup_entry;
			} else
#endif
			if (lstat(p->name, &st) != 0 ||
			    !S_ISDIR(st.st_mode)) {
				goto skip_fixup_entry;
			}
#endif
		}
		if (p->fixup & TODO_TIMES) {
			set_times(a, fd, p->mode, p->name,
			    p->atime, p->atime_nanos,
			    p->birthtime, p->birthtime_nanos,
			    p->mtime, p->mtime_nanos,
			    p->ctime, p->ctime_nanos);
		}
		if (p->fixup & TODO_MODE_BASE) {
#ifdef HAVE_FCHMOD
			if (fd >= 0)
				fchmod(fd, p->mode & 07777);
			else
#endif
#ifdef HAVE_LCHMOD
			lchmod(p->name, p->mode & 07777);
#else
			chmod(p->name, p->mode & 07777);
#endif
		}
		if (p->fixup & TODO_ACLS)
			archive_write_disk_set_acls(&a->archive, fd,
			    p->name, &p->acl, p->mode);
		if (p->fixup & TODO_FFLAGS)
			set_fflags_platform(a, fd, p->name,
			    p->mode, p->fflags_set, 0);
		if (p->fixup & TODO_MAC_METADATA)
			set_mac_metadata(a, p->name, p->mac_metadata,
					 p->mac_metadata_size);
skip_fixup_entry:
		next = p->next;
		archive_acl_clear(&p->acl);
		free(p->mac_metadata);
		free(p->name);
		if (fd >= 0)
			close(fd);
		free(p);
		p = next;
	}
	a->fixup_list = NULL;
	return (ret);
}