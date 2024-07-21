_handle_single_connection (int sockin, int sockout)
{
  const char *plugin_name;
  int ret = -1, r;
  struct connection *conn;
  int nworkers = threads ? threads : DEFAULT_PARALLEL_REQUESTS;
  pthread_t *workers = NULL;

  if (backend->thread_model (backend) < NBDKIT_THREAD_MODEL_PARALLEL ||
      nworkers == 1)
    nworkers = 0;
  conn = new_connection (sockin, sockout, nworkers);
  if (!conn)
    goto done;

  lock_request (conn);
  r = backend_open (backend, conn, read_only);
  unlock_request (conn);
  if (r == -1)
    goto done;

  /* NB: because of an asynchronous exit backend can be set to NULL at
   * just about any time.
   */
  if (backend)
    plugin_name = backend->plugin_name (backend);
  else
    plugin_name = "(unknown)";
  threadlocal_set_name (plugin_name);

  /* Prepare (for filters), called just after open. */
  lock_request (conn);
  if (backend)
    r = backend_prepare (backend, conn);
  else
    r = 0;
  unlock_request (conn);
  if (r == -1)
    goto done;

  /* Handshake. */
  if (protocol_handshake (conn) == -1)
    goto done;

  if (!nworkers) {
    /* No need for a separate thread. */
    debug ("handshake complete, processing requests serially");
    while (!quit && connection_get_status (conn) > 0)
      protocol_recv_request_send_reply (conn);
  }
  else {
    /* Create thread pool to process requests. */
    debug ("handshake complete, processing requests with %d threads",
           nworkers);
    workers = calloc (nworkers, sizeof *workers);
    if (!workers) {
      perror ("malloc");
      goto done;
    }

    for (nworkers = 0; nworkers < conn->nworkers; nworkers++) {
      struct worker_data *worker = malloc (sizeof *worker);
      int err;

      if (!worker) {
        perror ("malloc");
        connection_set_status (conn, -1);
        goto wait;
      }
      if (asprintf (&worker->name, "%s.%d", plugin_name, nworkers) < 0) {
        perror ("asprintf");
        connection_set_status (conn, -1);
        free (worker);
        goto wait;
      }
      worker->conn = conn;
      err = pthread_create (&workers[nworkers], NULL, connection_worker,
                            worker);
      if (err) {
        errno = err;
        perror ("pthread_create");
        connection_set_status (conn, -1);
        free (worker);
        goto wait;
      }
    }

  wait:
    while (nworkers)
      pthread_join (workers[--nworkers], NULL);
    free (workers);
  }

  /* Finalize (for filters), called just before close. */
  lock_request (conn);
  if (backend)
    r = backend->finalize (backend, conn);
  else
    r = 0;
  unlock_request (conn);
  if (r == -1)
    goto done;

  ret = connection_get_status (conn);
 done:
  free_connection (conn);
  return ret;
}