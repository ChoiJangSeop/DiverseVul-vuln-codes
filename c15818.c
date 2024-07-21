compress_write(ds_file_t *file, const uchar *buf, size_t len)
{
	ds_compress_file_t	*comp_file;
	ds_compress_ctxt_t	*comp_ctxt;
	comp_thread_ctxt_t	*threads;
	comp_thread_ctxt_t	*thd;
	uint			nthreads;
	uint			i;
	const char		*ptr;
	ds_file_t		*dest_file;

	comp_file = (ds_compress_file_t *) file->ptr;
	comp_ctxt = comp_file->comp_ctxt;
	dest_file = comp_file->dest_file;

	threads = comp_ctxt->threads;
	nthreads = comp_ctxt->nthreads;

	ptr = (const char *) buf;
	while (len > 0) {
		uint max_thread;

		/* Send data to worker threads for compression */
		for (i = 0; i < nthreads; i++) {
			size_t chunk_len;

			thd = threads + i;

			pthread_mutex_lock(&thd->data_mutex);

			chunk_len = (len > COMPRESS_CHUNK_SIZE) ?
				COMPRESS_CHUNK_SIZE : len;
			thd->from = ptr;
			thd->from_len = chunk_len;

			thd->data_avail = TRUE;
			pthread_cond_signal(&thd->data_cond);
			pthread_mutex_unlock(&thd->data_mutex);

			len -= chunk_len;
			if (len == 0) {
				break;
			}
			ptr += chunk_len;
		}

		max_thread = (i < nthreads) ? i :  nthreads - 1;

		/* Reap and stream the compressed data */
		for (i = 0; i <= max_thread; i++) {
			thd = threads + i;

			pthread_mutex_lock(&thd->data_mutex);
			while (thd->data_avail == TRUE) {
				pthread_cond_wait(&thd->data_cond,
						  &thd->data_mutex);
			}

			xb_a(threads[i].to_len > 0);

			if (ds_write(dest_file, "NEWBNEWB", 8) ||
			    write_uint64_le(dest_file,
					    comp_file->bytes_processed)) {
				msg("compress: write to the destination stream "
				    "failed.");
				return 1;
			}

			comp_file->bytes_processed += threads[i].from_len;

			if (write_uint32_le(dest_file, threads[i].adler) ||
			    ds_write(dest_file, threads[i].to,
					   threads[i].to_len)) {
				msg("compress: write to the destination stream "
				    "failed.");
				return 1;
			}

			pthread_mutex_unlock(&threads[i].data_mutex);
		}
	}

	return 0;
}