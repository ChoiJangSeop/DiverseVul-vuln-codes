add_account (GoaProvider    *provider,
             GoaClient      *client,
             GtkDialog      *dialog,
             GtkBox         *vbox,
             GError        **error)
{
  AddAccountData data;
  GVariantBuilder credentials;
  GVariantBuilder details;
  GoaEwsClient *ews_client;
  GoaObject *ret;
  const gchar *email_address;
  const gchar *server;
  const gchar *password;
  const gchar *username;
  const gchar *provider_type;
  gint response;

  ews_client = NULL;
  ret = NULL;

  memset (&data, 0, sizeof (AddAccountData));
  data.loop = g_main_loop_new (NULL, FALSE);
  data.dialog = dialog;
  data.error = NULL;

  create_account_details_ui (provider, dialog, vbox, TRUE, &data);
  gtk_widget_show_all (GTK_WIDGET (vbox));

  ews_client = goa_ews_client_new ();

 ews_again:
  response = gtk_dialog_run (dialog);
  if (response != GTK_RESPONSE_OK)
    {
      g_set_error (&data.error,
                   GOA_ERROR,
                   GOA_ERROR_DIALOG_DISMISSED,
                   _("Dialog was dismissed"));
      goto out;
    }

  email_address = gtk_entry_get_text (GTK_ENTRY (data.email_address));
  password = gtk_entry_get_text (GTK_ENTRY (data.password));
  username = gtk_entry_get_text (GTK_ENTRY (data.username));
  server = gtk_entry_get_text (GTK_ENTRY (data.server));

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

  goa_ews_client_autodiscover (ews_client,
                               email_address,
                               password,
                               username,
                               server,
                               NULL,
                               autodiscover_cb,
                               &data);
  goa_spinner_button_start (GOA_SPINNER_BUTTON (data.spinner_button));
  g_main_loop_run (data.loop);

  if (data.error != NULL)
    {
      gchar *markup;

      markup = g_strdup_printf ("<b>%s:</b> %s",
                                _("Error connecting to Microsoft Exchange server"),
                                data.error->message);
      g_clear_error (&data.error);

      gtk_label_set_markup (GTK_LABEL (data.cluebar_label), markup);
      g_free (markup);

      goa_spinner_button_set_label (GOA_SPINNER_BUTTON (data.spinner_button), _("_Try Again"));
      gtk_expander_set_expanded (GTK_EXPANDER (data.expander), TRUE);
      gtk_widget_set_no_show_all (data.cluebar, FALSE);
      gtk_widget_show_all (data.cluebar);
      goto ews_again;
    }

  gtk_widget_hide (GTK_WIDGET (dialog));

  g_variant_builder_init (&credentials, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&credentials, "{sv}", "password", g_variant_new_string (password));

  g_variant_builder_init (&details, G_VARIANT_TYPE ("a{ss}"));
  g_variant_builder_add (&details, "{ss}", "MailEnabled", "true");
  g_variant_builder_add (&details, "{ss}", "CalendarEnabled", "true");
  g_variant_builder_add (&details, "{ss}", "ContactsEnabled", "true");
  g_variant_builder_add (&details, "{ss}", "Host", server);

  /* OK, everything is dandy, add the account */
  /* we want the GoaClient to update before this method returns (so it
   * can create a proxy for the new object) so run the mainloop while
   * waiting for this to complete
   */
  goa_manager_call_add_account (goa_client_get_manager (client),
                                goa_provider_get_provider_type (provider),
                                username,
                                email_address,
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

  g_free (data.account_object_path);
  if (data.loop != NULL)
    g_main_loop_unref (data.loop);
  if (ews_client != NULL)
    g_object_unref (ews_client);
  return ret;
}