_archive_write_disk_close(struct archive *_a)
{
	struct archive_write_disk *a = (struct archive_write_disk *)_a;
	struct fixup_entry *next, *p;
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
		if (p->fixup &
		    (TODO_TIMES | TODO_MODE_BASE | TODO_ACLS | TODO_FFLAGS)) {
			fd = open(p->name,
			    O_WRONLY | O_BINARY | O_NOFOLLOW | O_CLOEXEC);
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
				fchmod(fd, p->mode);
			else
#endif
			chmod(p->name, p->mode);
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