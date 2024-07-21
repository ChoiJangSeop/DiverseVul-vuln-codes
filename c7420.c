qb_log_blackbox_write_to_file(const char *filename)
{
	ssize_t written_size = 0;
	struct qb_log_target *t;
	struct _blackbox_file_header header;
	int fd = open(filename, O_CREAT | O_RDWR | O_EXCL, 0700);

	if (fd < 0) {
		return -errno;
	}

	/* Write header, so we know this is a 'new' format blackbox */
	header.word_size = QB_BLACKBOX_HEADER_WORDSIZE;
	header.read_pt   = QB_BLACKBOX_HEADER_READPT;
	header.write_pt  = QB_BLACKBOX_HEADER_WRITEPT;
	header.version   = QB_BLACKBOX_HEADER_VERSION;
	header.hash      = QB_BLACKBOX_HEADER_HASH;
	written_size = write(fd, &header, sizeof(header));
	if (written_size < sizeof(header)) {
		close(fd);
		return written_size;
	}

	t = qb_log_target_get(QB_LOG_BLACKBOX);
	if (t->instance) {
		written_size += qb_rb_write_to_file(t->instance, fd);
	} else {
		written_size = -ENOENT;
	}
	close(fd);

	return written_size;
}