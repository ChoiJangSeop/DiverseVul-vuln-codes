verify_peer_certificate (GTlsConnectionBase *tls,
                         GTlsCertificate    *peer_certificate)
{
  GTlsConnectionBaseClass *tls_class = G_TLS_CONNECTION_BASE_GET_CLASS (tls);
  GSocketConnectable *peer_identity;
  GTlsDatabase *database;
  GTlsCertificateFlags errors;
  gboolean is_client;

  is_client = G_IS_TLS_CLIENT_CONNECTION (tls);

  if (!is_client)
    peer_identity = NULL;
  else if (!g_tls_connection_base_is_dtls (tls))
    peer_identity = g_tls_client_connection_get_server_identity (G_TLS_CLIENT_CONNECTION (tls));
  else
    peer_identity = g_dtls_client_connection_get_server_identity (G_DTLS_CLIENT_CONNECTION (tls));

  errors = 0;

  database = g_tls_connection_get_database (G_TLS_CONNECTION (tls));
  if (!database)
    {
      errors |= G_TLS_CERTIFICATE_UNKNOWN_CA;
      errors |= g_tls_certificate_verify (peer_certificate, peer_identity, NULL);
    }
  else
    {
      GError *error = NULL;

      errors |= g_tls_database_verify_chain (database, peer_certificate,
                                             is_client ?
                                             G_TLS_DATABASE_PURPOSE_AUTHENTICATE_SERVER :
                                             G_TLS_DATABASE_PURPOSE_AUTHENTICATE_CLIENT,
                                             peer_identity,
                                             g_tls_connection_get_interaction (G_TLS_CONNECTION (tls)),
                                             G_TLS_DATABASE_VERIFY_NONE,
                                             NULL, &error);
      if (error)
        {
          g_warning ("failure verifying certificate chain: %s",
                     error->message);
          g_assert (errors != 0);
          g_clear_error (&error);
        }
    }

  if (tls_class->verify_peer_certificate)
    errors |= tls_class->verify_peer_certificate (tls, peer_certificate, errors);

  return errors;
}