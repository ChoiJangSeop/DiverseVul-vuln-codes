ensure_credentials_sync (GoaProvider         *provider,
                         GoaObject           *object,
                         gint                *out_expires_in,
                         GCancellable        *cancellable,
                         GError             **error)
{
  GVariant *credentials;
  GoaAccount *account;
  GoaEwsClient *ews_client;
  GoaExchange *exchange;
  gboolean ret;
  const gchar *email_address;
  const gchar *server;
  const gchar *username;
  gchar *password;

  credentials = NULL;
  ews_client = NULL;
  password = NULL;

  ret = FALSE;

  credentials = goa_utils_lookup_credentials_sync (provider,
                                                   object,
                                                   cancellable,
                                                   error);
  if (credentials == NULL)
    {
      if (error != NULL)
        {
          g_prefix_error (error, _("Credentials not found in keyring (%s, %d): "),
                          g_quark_to_string ((*error)->domain), (*error)->code);
          (*error)->domain = GOA_ERROR;
          (*error)->code = GOA_ERROR_NOT_AUTHORIZED;
        }
      goto out;
    }

  account = goa_object_peek_account (object);
  email_address = goa_account_get_presentation_identity (account);
  username = goa_account_get_identity (account);

  if (!g_variant_lookup (credentials, "password", "s", &password))
    {
      if (error != NULL)
        {
          *error = g_error_new (GOA_ERROR,
                                GOA_ERROR_NOT_AUTHORIZED,
                                _("Did not find password with username `%s' in credentials"),
                                username);
        }
      goto out;
    }

  exchange = goa_object_peek_exchange (object);
  server = goa_exchange_get_host (exchange);

  ews_client = goa_ews_client_new ();
  ret = goa_ews_client_autodiscover_sync (ews_client,
                                          email_address,
                                          password,
                                          username,
                                          server,
                                          cancellable,
                                          error);
  if (!ret)
    {
      if (error != NULL)
        {
          g_prefix_error (error,
                          /* Translators: the first %s is the username
                           * (eg., debarshi.ray@gmail.com or rishi), and the
                           * (%s, %d) is the error domain and code.
                           */
                          _("Invalid password with username `%s' (%s, %d): "),
                          username,
                          g_quark_to_string ((*error)->domain),
                          (*error)->code);
          (*error)->domain = GOA_ERROR;
          (*error)->code = GOA_ERROR_NOT_AUTHORIZED;
        }
      goto out;
    }

  if (out_expires_in != NULL)
    *out_expires_in = 0;

 out:
  if (ews_client != NULL)
    g_object_unref (ews_client);
  g_free (password);
  if (credentials != NULL)
    g_variant_unref (credentials);
  return ret;
}