ensure_credentials_sync (GoaProvider         *provider,
                         GoaObject           *object,
                         gint                *out_expires_in,
                         GCancellable        *cancellable,
                         GError             **error)
{
  GVariant *credentials;
  GoaAccount *account;
  GoaHttpClient *http_client;
  gboolean ret;
  const gchar *username;
  gchar *password;
  gchar *uri;
  gchar *uri_webdav;

  credentials = NULL;
  http_client = NULL;
  password = NULL;
  uri = NULL;
  uri_webdav = NULL;

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

  uri = goa_util_lookup_keyfile_string (object, "Uri");
  uri_webdav = g_strconcat (uri, WEBDAV_ENDPOINT, NULL);

  account = goa_object_peek_account (object);
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

  http_client = goa_http_client_new ();
  ret = goa_http_client_check_sync (http_client,
                                    uri_webdav,
                                    username,
                                    password,
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
  g_clear_object (&http_client);
  g_free (password);
  g_free (uri);
  g_free (uri_webdav);
  if (credentials != NULL)
    g_variant_unref (credentials);
  return ret;
}