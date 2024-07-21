ensure_credentials_sync (GoaProvider   *provider,
                         GoaObject     *object,
                         gint          *out_expires_in,
                         GCancellable  *cancellable,
                         GError       **error)
{
  GVariant *credentials;
  GoaAccount *account;
  GoaHttpClient *http_client;
  gboolean ret;
  const gchar *username;
  gchar *password;
  gchar *uri_caldav;

  credentials = NULL;
  http_client = NULL;
  password = NULL;
  uri_caldav = NULL;

  ret = FALSE;

  /* Chain up */
  if (!GOA_PROVIDER_CLASS (goa_google_provider_parent_class)->ensure_credentials_sync (provider,
                                                                                       object,
                                                                                       out_expires_in,
                                                                                       cancellable,
                                                                                       error))
    goto out;

  credentials = goa_utils_lookup_credentials_sync (provider,
                                                   object,
                                                   cancellable,
                                                   error);
  if (credentials == NULL)
    {
      if (error != NULL)
        {
          g_prefix_error (error,
                          _("Credentials not found in keyring (%s, %d): "),
                          g_quark_to_string ((*error)->domain),
                          (*error)->code);
          (*error)->domain = GOA_ERROR;
          (*error)->code = GOA_ERROR_NOT_AUTHORIZED;
        }
      goto out;
    }

  account = goa_object_peek_account (object);
  username = goa_account_get_presentation_identity (account);
  uri_caldav = g_strdup_printf (CALDAV_ENDPOINT, username);

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
                                    uri_caldav,
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

 out:
  g_clear_object (&http_client);
  g_free (password);
  g_free (uri_caldav);
  g_clear_pointer (&credentials, (GDestroyNotify) g_variant_unref);
  return ret;
}