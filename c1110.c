refresh_account (GoaProvider    *provider,
                 GoaClient      *client,
                 GoaObject      *object,
                 GtkWindow      *parent,
                 GError        **error)
{
  AddAccountData data;
  GVariantBuilder builder;
  GoaAccount *account;
  GoaEwsClient *ews_client;
  GoaExchange *exchange;
  GtkWidget *dialog;
  GtkWidget *vbox;
  gboolean ret;
  const gchar *email_address;
  const gchar *server;
  const gchar *password;
  const gchar *username;
  gint response;

  g_return_val_if_fail (GOA_IS_EXCHANGE_PROVIDER (provider), FALSE);
  g_return_val_if_fail (GOA_IS_CLIENT (client), FALSE);
  g_return_val_if_fail (GOA_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ews_client = NULL;
  ret = FALSE;

  dialog = gtk_dialog_new_with_buttons (NULL,
                                        parent,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        NULL);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 12);
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_set_spacing (GTK_BOX (vbox), 12);

  memset (&data, 0, sizeof (AddAccountData));
  data.cancellable = g_cancellable_new ();
  data.loop = g_main_loop_new (NULL, FALSE);
  data.dialog = GTK_DIALOG (dialog);
  data.error = NULL;

  create_account_details_ui (provider, GTK_DIALOG (dialog), GTK_BOX (vbox), FALSE, &data);

  account = goa_object_peek_account (object);
  email_address = goa_account_get_presentation_identity (account);
  gtk_entry_set_text (GTK_ENTRY (data.email_address), email_address);
  gtk_editable_set_editable (GTK_EDITABLE (data.email_address), FALSE);

  gtk_widget_show_all (dialog);
  g_signal_connect (dialog, "response", G_CALLBACK (dialog_response_cb), &data);

  ews_client = goa_ews_client_new ();

 ews_again:
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response != GTK_RESPONSE_OK)
    {
      g_set_error (&data.error,
                   GOA_ERROR,
                   GOA_ERROR_DIALOG_DISMISSED,
                   _("Dialog was dismissed"));
      goto out;
    }

  password = gtk_entry_get_text (GTK_ENTRY (data.password));
  username = goa_account_get_identity (account);

  exchange = goa_object_peek_exchange (object);
  server = goa_exchange_get_host (exchange);

  g_cancellable_reset (data.cancellable);
  goa_ews_client_autodiscover (ews_client,
                               email_address,
                               password,
                               username,
                               server,
                               data.cancellable,
                               autodiscover_cb,
                               &data);
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
                                _("Error connecting to Microsoft Exchange server"),
                                data.error->message);
      g_clear_error (&data.error);

      gtk_label_set_markup (GTK_LABEL (data.cluebar_label), markup);
      g_free (markup);

      gtk_button_set_label (GTK_BUTTON (data.connect_button), _("_Try Again"));
      gtk_widget_set_no_show_all (data.cluebar, FALSE);
      gtk_widget_show_all (data.cluebar);
      goto ews_again;
    }

  /* TODO: run in worker thread */
  g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&builder, "{sv}", "password", g_variant_new_string (password));

  if (!goa_utils_store_credentials_for_object_sync (provider,
                                                    object,
                                                    g_variant_builder_end (&builder),
                                                    NULL, /* GCancellable */
                                                    &data.error))
    goto out;

  goa_account_call_ensure_credentials (account,
                                       NULL, /* GCancellable */
                                       NULL, NULL); /* callback, user_data */

  ret = TRUE;

 out:
  if (data.error != NULL)
    g_propagate_error (error, data.error);

  gtk_widget_destroy (dialog);
  if (data.loop != NULL)
    g_main_loop_unref (data.loop);
  g_clear_object (&data.cancellable);
  if (ews_client != NULL)
    g_object_unref (ews_client);
  return ret;
}