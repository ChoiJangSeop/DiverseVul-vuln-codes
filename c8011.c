free_connection (struct connection *conn)
{
  if (!conn)
    return;

  threadlocal_set_conn (NULL);
  conn->close (conn);
  if (listen_stdin) {
    int fd;

    /* Restore something to stdin/out so the rest of our code can
     * continue to assume that all new fds will be above stderr.
     * Swap directions to get EBADF on improper use of stdin/out.
     */
    fd = open ("/dev/null", O_WRONLY | O_CLOEXEC);
    assert (fd == 0);
    fd = open ("/dev/null", O_RDONLY | O_CLOEXEC);
    assert (fd == 1);
  }

  /* Don't call the plugin again if quit has been set because the main
   * thread will be in the process of unloading it.  The plugin.unload
   * callback should always be called.
   */
  if (!quit && connection_get_handle (conn, 0)) {
    lock_request (conn);
    backend->close (backend, conn);
    unlock_request (conn);
  }

  if (conn->status_pipe[0] >= 0) {
    close (conn->status_pipe[0]);
    close (conn->status_pipe[1]);
  }

  pthread_mutex_destroy (&conn->request_lock);
  pthread_mutex_destroy (&conn->read_lock);
  pthread_mutex_destroy (&conn->write_lock);
  pthread_mutex_destroy (&conn->status_lock);

  free (conn->handles);
  free (conn->exportname);
  free (conn);
}