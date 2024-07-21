qb_rb_open_2(const char *name, size_t size, uint32_t flags,
	     size_t shared_user_data_size,
	     struct qb_rb_notifier *notifiers)
{
	struct qb_ringbuffer_s *rb;
	size_t real_size;
	size_t shared_size;
	char path[PATH_MAX];
	int32_t fd_hdr;
	int32_t fd_data;
	uint32_t file_flags = O_RDWR;
	char filename[PATH_MAX];
	int32_t error = 0;
	void *shm_addr;
	long page_size = sysconf(_SC_PAGESIZE);

#ifdef QB_ARCH_HPPA
	page_size = QB_MAX(page_size, 0x00400000); /* align to page colour */
#elif defined(QB_FORCE_SHM_ALIGN)
	page_size = QB_MAX(page_size, 16 * 1024);
#endif /* QB_FORCE_SHM_ALIGN */
	/* The user of this api expects the 'size' parameter passed into this function
	 * to be reflective of the max size single write we can do to the 
	 * ringbuffer.  This means we have to add both the 'margin' space used
	 * to calculate if there is enough space for a new chunk as well as the '+1' that
	 * prevents overlap of the read/write pointers */
	size += QB_RB_CHUNK_MARGIN + 1;
	real_size = QB_ROUNDUP(size, page_size);

	shared_size =
	    sizeof(struct qb_ringbuffer_shared_s) + shared_user_data_size;

	if (flags & QB_RB_FLAG_CREATE) {
		file_flags |= O_CREAT | O_TRUNC;
	}

	rb = calloc(1, sizeof(struct qb_ringbuffer_s));
	if (rb == NULL) {
		return NULL;
	}

	/*
	 * Create a shared_hdr memory segment for the header.
	 */
	snprintf(filename, PATH_MAX, "qb-%s-header", name);
	fd_hdr = qb_sys_mmap_file_open(path, filename,
				       shared_size, file_flags);
	if (fd_hdr < 0) {
		error = fd_hdr;
		qb_util_log(LOG_ERR, "couldn't create file for mmap");
		goto cleanup_hdr;
	}

	rb->shared_hdr = mmap(0,
			      shared_size,
			      PROT_READ | PROT_WRITE, MAP_SHARED, fd_hdr, 0);

	if (rb->shared_hdr == MAP_FAILED) {
		error = -errno;
		qb_util_log(LOG_ERR, "couldn't create mmap for header");
		goto cleanup_hdr;
	}
	qb_atomic_init();

	rb->flags = flags;

	/*
	 * create the semaphore
	 */
	if (flags & QB_RB_FLAG_CREATE) {
		rb->shared_data = NULL;
		/* rb->shared_hdr->word_size tracks data by ints and not bytes/chars. */
		rb->shared_hdr->word_size = real_size / sizeof(uint32_t);
		rb->shared_hdr->write_pt = 0;
		rb->shared_hdr->read_pt = 0;
		(void)strlcpy(rb->shared_hdr->hdr_path, path, PATH_MAX);
	}
	if (notifiers && notifiers->post_fn) {
		error = 0;
		memcpy(&rb->notifier,
		       notifiers,
		       sizeof(struct qb_rb_notifier));
	} else {
		error = qb_rb_sem_create(rb, flags);
	}
	if (error < 0) {
		errno = -error;
		qb_util_perror(LOG_ERR, "couldn't create a semaphore");
		goto cleanup_hdr;
	}

	/* Create the shared_data memory segment for the actual ringbuffer.
	 * They have to be separate.
	 */
	if (flags & QB_RB_FLAG_CREATE) {
		snprintf(filename, PATH_MAX, "qb-%s-data", name);
		fd_data = qb_sys_mmap_file_open(path,
						filename,
						real_size, file_flags);
		(void)strlcpy(rb->shared_hdr->data_path, path, PATH_MAX);
	} else {
		fd_data = qb_sys_mmap_file_open(path,
						rb->shared_hdr->data_path,
						real_size, file_flags);
	}
	if (fd_data < 0) {
		error = fd_data;
		qb_util_log(LOG_ERR, "couldn't create file for mmap");
		goto cleanup_hdr;
	}

	qb_util_log(LOG_DEBUG,
		    "shm size:%ld; real_size:%ld; rb->word_size:%d", size,
		    real_size, rb->shared_hdr->word_size);

	/* this function closes fd_data */
	error = qb_sys_circular_mmap(fd_data, &shm_addr, real_size);
	rb->shared_data = shm_addr;
	if (error != 0) {
		qb_util_log(LOG_ERR, "couldn't create circular mmap on %s",
			    rb->shared_hdr->data_path);
		goto cleanup_data;
	}

	if (flags & QB_RB_FLAG_CREATE) {
		memset(rb->shared_data, 0, real_size);
		rb->shared_data[rb->shared_hdr->word_size] = 5;
		rb->shared_hdr->ref_count = 1;
	} else {
		qb_atomic_int_inc(&rb->shared_hdr->ref_count);
	}

	close(fd_hdr);
	return rb;

cleanup_data:
	if (flags & QB_RB_FLAG_CREATE) {
		unlink(rb->shared_hdr->data_path);
	}

cleanup_hdr:
	if (fd_hdr >= 0) {
		close(fd_hdr);
	}
	if (rb && (flags & QB_RB_FLAG_CREATE)) {
		unlink(rb->shared_hdr->hdr_path);
		if (rb->notifier.destroy_fn) {
			(void)rb->notifier.destroy_fn(rb->notifier.instance);
		}
	}
	if (rb && (rb->shared_hdr != MAP_FAILED && rb->shared_hdr != NULL)) {
		munmap(rb->shared_hdr, sizeof(struct qb_ringbuffer_shared_s));
	}
	free(rb);
	errno = -error;
	return NULL;
}