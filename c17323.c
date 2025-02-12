create_worker_threads(uint n)
{
	comp_thread_ctxt_t	*threads;
	uint 			i;

	threads = (comp_thread_ctxt_t *)
		my_malloc(sizeof(comp_thread_ctxt_t) * n, MYF(MY_FAE));

	for (i = 0; i < n; i++) {
		comp_thread_ctxt_t *thd = threads + i;

		thd->num = i + 1;
		thd->started = FALSE;
		thd->cancelled = FALSE;
		thd->data_avail = FALSE;

		thd->to = (char *) my_malloc(COMPRESS_CHUNK_SIZE +
						   MY_QLZ_COMPRESS_OVERHEAD,
						   MYF(MY_FAE));

		/* Initialize the control mutex and condition var */
		if (pthread_mutex_init(&thd->ctrl_mutex, NULL) ||
		    pthread_cond_init(&thd->ctrl_cond, NULL)) {
			goto err;
		}

		/* Initialize and data mutex and condition var */
		if (pthread_mutex_init(&thd->data_mutex, NULL) ||
		    pthread_cond_init(&thd->data_cond, NULL)) {
			goto err;
		}

		pthread_mutex_lock(&thd->ctrl_mutex);

		if (pthread_create(&thd->id, NULL, compress_worker_thread_func,
				   thd)) {
			msg("compress: pthread_create() failed: "
			    "errno = %d", errno);
			goto err;
		}
	}

	/* Wait for the threads to start */
	for (i = 0; i < n; i++) {
		comp_thread_ctxt_t *thd = threads + i;

		while (thd->started == FALSE)
			pthread_cond_wait(&thd->ctrl_cond, &thd->ctrl_mutex);
		pthread_mutex_unlock(&thd->ctrl_mutex);
	}

	return threads;

err:
	my_free(threads);
	return NULL;
}