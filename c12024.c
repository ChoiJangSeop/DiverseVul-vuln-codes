nbd_create (void)
{
  static _Atomic int hnums = 1;
  struct nbd_handle *h;
  const char *s;

  nbd_internal_set_error_context ("nbd_create");

  h = calloc (1, sizeof *h);
  if (h == NULL) {
    set_error (errno, "calloc");
    goto error1;
  }

  if (asprintf (&h->hname, "nbd%d", hnums++) == -1) {
    set_error (errno, "asprintf");
    goto error1;
  }

  h->unique = 1;
  h->tls_verify_peer = true;
  h->request_sr = true;

  h->uri_allow_transports = LIBNBD_ALLOW_TRANSPORT_MASK;
  h->uri_allow_tls = LIBNBD_TLS_ALLOW;
  h->uri_allow_local_file = false;

  h->gflags = LIBNBD_HANDSHAKE_FLAG_MASK;

  s = getenv ("LIBNBD_DEBUG");
  h->debug = s && strcmp (s, "1") == 0;

  h->strict = LIBNBD_STRICT_MASK;

  h->public_state = STATE_START;
  h->state = STATE_START;
  h->pid = -1;

  h->export_name = strdup ("");
  if (h->export_name == NULL) {
    set_error (errno, "strdup");
    goto error1;
  }

  errno = pthread_mutex_init (&h->lock, NULL);
  if (errno != 0) {
    set_error (errno, "pthread_mutex_init");
    goto error1;
  }

  if (nbd_internal_run (h, cmd_create) == -1)
    goto error2;

  debug (h, "opening handle");
  /*debug (h, "sizeof *h = %zu", sizeof *h);*/
  return h;

 error2:
  pthread_mutex_destroy (&h->lock);
 error1:
  if (h) {
    free (h->export_name);
    free (h->hname);
    free (h);
  }
  return NULL;
}