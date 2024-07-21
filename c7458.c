
int main(int argc, char **argv) {
  const char *host = (argc > 1) ? argv[1] : "";
  const char *port = (argc > 2) ? argv[2] : "amqp";

  broker_t b = {0};
  b.proactor = pn_proactor();
  queues_init(&b.queues);
  b.container_id = argv[0];
  b.threads = 4;
  b.ssl_domain = pn_ssl_domain(PN_SSL_MODE_SERVER);
  SET_CREDENTIALS(b.ssl_domain, "tserver");
  pn_ssl_domain_allow_unsecured_client(b.ssl_domain); /* Allow SSL and plain connections */
  {
  /* Listen on addr */
  char addr[PN_MAX_ADDR];
  pn_proactor_addr(addr, sizeof(addr), host, port);
  pn_proactor_listen(b.proactor, pn_listener(), addr, 16);
  }

  {
  /* Start n-1 threads */
  pthread_t* threads = (pthread_t*)calloc(sizeof(pthread_t), b.threads);
  size_t i;
  for (i = 0; i < b.threads-1; ++i) {
    pthread_create(&threads[i], NULL, broker_thread, &b);
  }
  broker_thread(&b);            /* Use the main thread too. */
  /* Join the other threads */
  for (i = 0; i < b.threads-1; ++i) {
    pthread_join(threads[i], NULL);
  }
  pn_proactor_free(b.proactor);
  free(threads);
  pn_ssl_domain_free(b.ssl_domain);
  return 0;
  }