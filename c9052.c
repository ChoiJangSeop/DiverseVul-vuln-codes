main (int   argc,
      char *argv[])
{
  int ret;

  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("http://bugzilla.gnome.org/");

  g_setenv ("GSETTINGS_BACKEND", "memory", TRUE);
  g_setenv ("GIO_USE_TLS", BACKEND, TRUE);

  g_assert_true (g_ascii_strcasecmp (G_OBJECT_TYPE_NAME (g_tls_backend_get_default ()), "GTlsBackend" BACKEND) == 0);

  g_test_add ("/tls/" BACKEND "/connection/basic", TestConnection, NULL,
              setup_connection, test_basic_connection, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/verified", TestConnection, NULL,
              setup_connection, test_verified_connection, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/verified-chain", TestConnection, NULL,
              setup_connection, test_verified_chain, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/verified-chain-with-redundant-root-cert", TestConnection, NULL,
              setup_connection, test_verified_chain_with_redundant_root_cert, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/verified-chain-with-duplicate-server-cert", TestConnection, NULL,
              setup_connection, test_verified_chain_with_duplicate_server_cert, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/verified-unordered-chain", TestConnection, NULL,
              setup_connection, test_verified_unordered_chain, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/verified-chain-with-alternative-ca-cert", TestConnection, NULL,
              setup_connection, test_verified_chain_with_alternative_ca_cert, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/invalid-chain-with-alternative-ca-cert", TestConnection, NULL,
              setup_connection, test_invalid_chain_with_alternative_ca_cert, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/client-auth", TestConnection, NULL,
              setup_connection, test_client_auth_connection, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/client-auth-rehandshake", TestConnection, NULL,
              setup_connection, test_client_auth_rehandshake, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/client-auth-failure", TestConnection, NULL,
              setup_connection, test_client_auth_failure, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/client-auth-fail-missing-client-private-key", TestConnection, NULL,
              setup_connection, test_client_auth_fail_missing_client_private_key, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/client-auth-request-cert", TestConnection, NULL,
              setup_connection, test_client_auth_request_cert, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/client-auth-request-fail", TestConnection, NULL,
              setup_connection, test_client_auth_request_fail, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/client-auth-request-none", TestConnection, NULL,
              setup_connection, test_client_auth_request_none, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/no-database", TestConnection, NULL,
              setup_connection, test_connection_no_database, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/failed", TestConnection, NULL,
              setup_connection, test_failed_connection, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/socket-client", TestConnection, NULL,
              setup_connection, test_connection_socket_client, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/socket-client-failed", TestConnection, NULL,
              setup_connection, test_connection_socket_client_failed, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/read-time-out-then-write", TestConnection, NULL,
              setup_connection, test_connection_read_time_out_write, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/simultaneous-async", TestConnection, NULL,
              setup_connection, test_simultaneous_async, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/simultaneous-sync", TestConnection, NULL,
              setup_connection, test_simultaneous_sync, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/simultaneous-async-rehandshake", TestConnection, NULL,
              setup_connection, test_simultaneous_async_rehandshake, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/simultaneous-sync-rehandshake", TestConnection, NULL,
              setup_connection, test_simultaneous_sync_rehandshake, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/close-immediately", TestConnection, NULL,
              setup_connection, test_close_immediately, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/unclean-close-by-server", TestConnection, NULL,
              setup_connection, test_unclean_close_by_server, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/async-implicit-handshake", TestConnection, NULL,
              setup_connection, test_async_implicit_handshake, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/output-stream-close", TestConnection, NULL,
              setup_connection, test_output_stream_close, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/fallback", TestConnection, NULL,
              setup_connection, test_fallback, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/garbage-database", TestConnection, NULL,
              setup_connection, test_garbage_database, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/readwrite-after-connection-destroyed", TestConnection, NULL,
              setup_connection, test_readwrite_after_connection_destroyed, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/alpn/match", TestConnection, NULL,
              setup_connection, test_alpn_match, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/alpn/no-match", TestConnection, NULL,
              setup_connection, test_alpn_no_match, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/alpn/client-only", TestConnection, NULL,
              setup_connection, test_alpn_client_only, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/alpn/server-only", TestConnection, NULL,
              setup_connection, test_alpn_server_only, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/sync-op-during-handshake", TestConnection, NULL,
              setup_connection, test_sync_op_during_handshake, teardown_connection);
  g_test_add ("/tls/" BACKEND "/connection/socket-timeout", TestConnection, NULL,
              setup_connection, test_socket_timeout, teardown_connection);

  ret = g_test_run ();

  /* for valgrinding */
  g_main_context_unref (g_main_context_default ());

  return ret;
}