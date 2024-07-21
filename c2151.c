evbuffer_file_segment_new(
	int fd, ev_off_t offset, ev_off_t length, unsigned flags)
{
	struct evbuffer_file_segment *seg =
	    mm_calloc(sizeof(struct evbuffer_file_segment), 1);
	if (!seg)
		return NULL;
	seg->refcnt = 1;
	seg->fd = fd;
	seg->flags = flags;
	seg->file_offset = offset;
	seg->cleanup_cb = NULL;
	seg->cleanup_cb_arg = NULL;
#ifdef _WIN32
#ifndef lseek
#define lseek _lseeki64
#endif
#ifndef fstat
#define fstat _fstat
#endif
#ifndef stat
#define stat _stat
#endif
#endif
	if (length == -1) {
		struct stat st;
		if (fstat(fd, &st) < 0)
			goto err;
		length = st.st_size;
	}
	seg->length = length;

#if defined(USE_SENDFILE)
	if (!(flags & EVBUF_FS_DISABLE_SENDFILE)) {
		seg->can_sendfile = 1;
		goto done;
	}
#endif

	if (evbuffer_file_segment_materialize(seg)<0)
		goto err;

#if defined(USE_SENDFILE)
done:
#endif
	if (!(flags & EVBUF_FS_DISABLE_LOCKING)) {
		EVTHREAD_ALLOC_LOCK(seg->lock, 0);
	}
	return seg;
err:
	mm_free(seg);
	return NULL;
}