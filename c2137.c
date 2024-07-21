evbuffer_add_file(struct evbuffer *outbuf, int fd,
    ev_off_t offset, ev_off_t length)
{
#if defined(USE_SENDFILE) || defined(_EVENT_HAVE_MMAP)
	struct evbuffer_chain *chain;
	struct evbuffer_chain_fd *info;
#endif
#if defined(USE_SENDFILE)
	int sendfile_okay = 1;
#endif
	int ok = 1;

#if defined(USE_SENDFILE)
	if (use_sendfile) {
		EVBUFFER_LOCK(outbuf);
		sendfile_okay = outbuf->flags & EVBUFFER_FLAG_DRAINS_TO_FD;
		EVBUFFER_UNLOCK(outbuf);
	}

	if (use_sendfile && sendfile_okay) {
		chain = evbuffer_chain_new(sizeof(struct evbuffer_chain_fd));
		if (chain == NULL) {
			event_warn("%s: out of memory", __func__);
			return (-1);
		}

		chain->flags |= EVBUFFER_SENDFILE | EVBUFFER_IMMUTABLE;
		chain->buffer = NULL;	/* no reading possible */
		chain->buffer_len = length + offset;
		chain->off = length;
		chain->misalign = offset;

		info = EVBUFFER_CHAIN_EXTRA(struct evbuffer_chain_fd, chain);
		info->fd = fd;

		EVBUFFER_LOCK(outbuf);
		if (outbuf->freeze_end) {
			mm_free(chain);
			ok = 0;
		} else {
			outbuf->n_add_for_cb += length;
			evbuffer_chain_insert(outbuf, chain);
		}
	} else
#endif
#if defined(_EVENT_HAVE_MMAP)
	if (use_mmap) {
		void *mapped = mmap(NULL, length + offset, PROT_READ,
#ifdef MAP_NOCACHE
		    MAP_NOCACHE |
#endif
#ifdef MAP_FILE
		    MAP_FILE |
#endif
		    MAP_PRIVATE,
		    fd, 0);
		/* some mmap implementations require offset to be a multiple of
		 * the page size.  most users of this api, are likely to use 0
		 * so mapping everything is not likely to be a problem.
		 * TODO(niels): determine page size and round offset to that
		 * page size to avoid mapping too much memory.
		 */
		if (mapped == MAP_FAILED) {
			event_warn("%s: mmap(%d, %d, %zu) failed",
			    __func__, fd, 0, (size_t)(offset + length));
			return (-1);
		}
		chain = evbuffer_chain_new(sizeof(struct evbuffer_chain_fd));
		if (chain == NULL) {
			event_warn("%s: out of memory", __func__);
			munmap(mapped, length);
			return (-1);
		}

		chain->flags |= EVBUFFER_MMAP | EVBUFFER_IMMUTABLE;
		chain->buffer = mapped;
		chain->buffer_len = length + offset;
		chain->off = length + offset;

		info = EVBUFFER_CHAIN_EXTRA(struct evbuffer_chain_fd, chain);
		info->fd = fd;

		EVBUFFER_LOCK(outbuf);
		if (outbuf->freeze_end) {
			info->fd = -1;
			evbuffer_chain_free(chain);
			ok = 0;
		} else {
			outbuf->n_add_for_cb += length;

			evbuffer_chain_insert(outbuf, chain);

			/* we need to subtract whatever we don't need */
			evbuffer_drain(outbuf, offset);
		}
	} else
#endif
	{
		/* the default implementation */
		struct evbuffer *tmp = evbuffer_new();
		ev_ssize_t read;

		if (tmp == NULL)
			return (-1);

#ifdef WIN32
#define lseek _lseeki64
#endif
		if (lseek(fd, offset, SEEK_SET) == -1) {
			evbuffer_free(tmp);
			return (-1);
		}

		/* we add everything to a temporary buffer, so that we
		 * can abort without side effects if the read fails.
		 */
		while (length) {
			read = evbuffer_readfile(tmp, fd, (ev_ssize_t)length);
			if (read == -1) {
				evbuffer_free(tmp);
				return (-1);
			}

			length -= read;
		}

		EVBUFFER_LOCK(outbuf);
		if (outbuf->freeze_end) {
			evbuffer_free(tmp);
			ok = 0;
		} else {
			evbuffer_add_buffer(outbuf, tmp);
			evbuffer_free(tmp);

#ifdef WIN32
#define close _close
#endif
			close(fd);
		}
	}

	if (ok)
		evbuffer_invoke_callbacks(outbuf);
	EVBUFFER_UNLOCK(outbuf);

	return ok ? 0 : -1;
}