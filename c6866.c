int journal_remote_handle_raw_source(
                sd_event_source *event,
                int fd,
                uint32_t revents,
                RemoteServer *s) {

        RemoteSource *source;
        int r;

        /* Returns 1 if there might be more data pending,
         * 0 if data is currently exhausted, negative on error.
         */

        assert(fd >= 0 && fd < (ssize_t) s->sources_size);
        source = s->sources[fd];
        assert(source->importer.fd == fd);

        r = process_source(source, s->compress, s->seal);
        if (journal_importer_eof(&source->importer)) {
                size_t remaining;

                log_debug("EOF reached with source %s (fd=%d)",
                          source->importer.name, source->importer.fd);

                remaining = journal_importer_bytes_remaining(&source->importer);
                if (remaining > 0)
                        log_notice("Premature EOF. %zu bytes lost.", remaining);
                remove_source(s, source->importer.fd);
                log_debug("%zu active sources remaining", s->active);
                return 0;
        } else if (r == -E2BIG) {
                log_notice("Entry too big, skipped");
                return 1;
        } else if (r == -EAGAIN) {
                return 0;
        } else if (r < 0) {
                log_debug_errno(r, "Closing connection: %m");
                remove_source(s, fd);
                return 0;
        } else
                return 1;
}