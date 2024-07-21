add_account (GoaProvider    *provider,
             GoaClient      *client,
             GtkDialog      *dialog,
             GtkBox         *vbox,
             GError        **error)
{
  AddAccountData data;
  GVariantBuilder credentials;
  GVariantBuilder details;
  GoaHttpClient *http_client;
  GoaObject *ret;
  const gchar *uri_text;
  const gchar *password;
  const gchar *username;
  const gchar *provider_type;
  gchar *presentation_identity;
  gchar *server;
  gchar *uri;
  gchar *uri_webdav;
  gint response;

  http_client = NULL;
  presentation_identity = NULL;
  server = NULL;
  uri = NULL;

  ret = NULL;

  memset (&data, 0, sizeof (AddAccountData));
  data.cancellable = g_cancellable_new ();
  data.loop = g_main_loop_new (NULL, FALSE);
  data.dialog = dialog;
  data.error = NULL;

  create_account_details_ui (provider, dialog, vbox, TRUE, &data);
  gtk_widget_show_all (GTK_WIDGET (vbox));
  g_signal_connect (dialog, "response", G_CALLBACK (dialog_response_cb), &data);

  http_client = goa_http_client_new ();

 http_again:
  response = gtk_dialog_run (dialog);
  if (response != GTK_RESPONSE_OK)
    {
      g_set_error (&data.error,
                   GOA_ERROR,
                   GOA_ERROR_DIALOG_DISMISSED,
                   _("Dialog was dismissed"));
      goto out;
    }

  uri_text = gtk_entry_get_text (GTK_ENTRY (data.uri));
  username = gtk_entry_get_text (GTK_ENTRY (data.username));
  password = gtk_entry_get_text (GTK_ENTRY (data.password));

  /* See if there's already an account of this type with the
   * given identity
   */
  provider_type = goa_provider_get_provider_type (provider);
  if (!goa_utils_check_duplicate (client,
                                  username,
                                  provider_type,
                                  (GoaPeekInterfaceFunc) goa_object_peek_password_based,
                                  &data.error))
    goto out;

  uri = normalize_uri (uri_text, &server);
  uri_webdav = g_strconcat (uri, WEBDAV_ENDPOINT, NULL);
  g_cancellable_reset (data.cancellable);
  goa_http_client_check (http_client,
                         uri_webdav,
                         username,
                         password,
                         data.cancellable,
                         check_cb,
                         &data);
  g_free (uri_webdav);

  gtk_widget_set_sensitive (data.connect_button, FALSE);
  gtk_widget_show (data.progress_grid);
  g_main_loop_run (data.loop);

  if (g_cancellable_is_cancelled (data.cancellable))
    {
      g_prefix_error (&data.error,
                      _("Dialog was dismissed (%s, %d): "),
                      g_quark_to_string (data.error->domain),
                      data.error->code);
      data.error->domain = GOA_ERROR;
      data.error->code = GOA_ERROR_DIALOG_DISMISSED;
      goto out;
    }
  else if (data.error != NULL)
    {
      gchar *markup;

      markup = g_strdup_printf ("<b>%s:</b> %s",
                                _("Error connecting to ownCloud server"),
                                data.error->message);
      g_clear_error (&data.error);

      gtk_label_set_markup (GTK_LABEL (data.cluebar_label), markup);
      g_free (markup);

      gtk_button_set_label (GTK_BUTTON (data.connect_button), _("_Try Again"));
      gtk_widget_set_no_show_all (data.cluebar, FALSE);
      gtk_widget_show_all (data.cluebar);

      g_clear_pointer (&server, g_free);
      g_clear_pointer (&uri, g_free);
      goto http_again;
    }

  gtk_widget_hide (GTK_WIDGET (dialog));

  g_variant_builder_init (&credentials, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&credentials, "{sv}", "password", g_variant_new_string (password));

  g_variant_builder_init (&details, G_VARIANT_TYPE ("a{ss}"));
  g_variant_builder_add (&details, "{ss}", "CalendarEnabled", "true");
  g_variant_builder_add (&details, "{ss}", "ContactsEnabled", "true");
  g_variant_builder_add (&details, "{ss}", "FilesEnabled", "true");
  g_variant_builder_add (&details, "{ss}", "Uri", uri);

  /* OK, everything is dandy, add the account */
  /* we want the GoaClient to update before this method returns (so it
   * can create a proxy for the new object) so run the mainloop while
   * waiting for this to complete
   */
  presentation_identity = g_strconcat (username, "@", server, NULL);
  goa_manager_call_add_account (goa_client_get_manager (client),
                                goa_provider_get_provider_type (provider),
                                username,
                                presentation_identity,
                                g_variant_builder_end (&credentials),
                                g_variant_builder_end (&details),
                                NULL, /* GCancellable* */
                                (GAsyncReadyCallback) add_account_cb,
                                &data);
  g_main_loop_run (data.loop);
  if (data.error != NULL)
    goto out;

  ret = GOA_OBJECT (g_dbus_object_manager_get_object (goa_client_get_object_manager (client),
                                                      data.account_object_path));

 out:
  /* We might have an object even when data.error is set.
   * eg., if we failed to store the credentials in the keyring.
   */
  if (data.error != NULL)
    g_propagate_error (error, data.error);
  else
    g_assert (ret != NULL);

  g_free (presentation_identity);
  g_free (server);
  g_free (uri);
  g_free (data.account_object_path);
  if (data.loop != NULL)
    g_main_loop_unref (data.loop);
  g_clear_object (&data.cancellable);
  g_clear_object (&http_client);
  return ret;
}