static int get_line(JournalImporter *imp, char **line, size_t *size) {
        ssize_t n;
        char *c = NULL;

        assert(imp);
        assert(imp->state == IMPORTER_STATE_LINE);
        assert(imp->offset <= imp->filled);
        assert(imp->filled <= imp->size);
        assert(!imp->buf || imp->size > 0);
        assert(imp->fd >= 0);

        for (;;) {
                if (imp->buf) {
                        size_t start = MAX(imp->scanned, imp->offset);

                        c = memchr(imp->buf + start, '\n',
                                   imp->filled - start);
                        if (c != NULL)
                                break;
                }

                imp->scanned = imp->filled;
                if (imp->scanned >= DATA_SIZE_MAX)
                        return log_error_errno(SYNTHETIC_ERRNO(E2BIG),
                                               "Entry is bigger than %u bytes.",
                                               DATA_SIZE_MAX);

                if (imp->passive_fd)
                        /* we have to wait for some data to come to us */
                        return -EAGAIN;

                /* We know that imp->filled is at most DATA_SIZE_MAX, so if
                   we reallocate it, we'll increase the size at least a bit. */
                assert_cc(DATA_SIZE_MAX < ENTRY_SIZE_MAX);
                if (imp->size - imp->filled < LINE_CHUNK &&
                    !realloc_buffer(imp, MIN(imp->filled + LINE_CHUNK, ENTRY_SIZE_MAX)))
                                return log_oom();

                assert(imp->buf);
                assert(imp->size - imp->filled >= LINE_CHUNK ||
                       imp->size == ENTRY_SIZE_MAX);

                n = read(imp->fd,
                         imp->buf + imp->filled,
                         imp->size - imp->filled);
                if (n < 0) {
                        if (errno != EAGAIN)
                                log_error_errno(errno, "read(%d, ..., %zu): %m",
                                                imp->fd,
                                                imp->size - imp->filled);
                        return -errno;
                } else if (n == 0)
                        return 0;

                imp->filled += n;
        }

        *line = imp->buf + imp->offset;
        *size = c + 1 - imp->buf - imp->offset;
        imp->offset += *size;

        return 1;
}