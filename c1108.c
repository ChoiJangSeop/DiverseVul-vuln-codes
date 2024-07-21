build_object (GoaProvider         *provider,
              GoaObjectSkeleton   *object,
              GKeyFile            *key_file,
              const gchar         *group,
              GDBusConnection     *connection,
              gboolean             just_added,
              GError             **error)
{
  GoaAccount *account;
  GoaCalendar *calendar;
  GoaContacts *contacts;
  GoaExchange *exchange;
  GoaMail *mail;
  GoaPasswordBased *password_based;
  gboolean calendar_enabled;
  gboolean contacts_enabled;
  gboolean mail_enabled;
  gboolean ret;

  account = NULL;
  calendar = NULL;
  contacts = NULL;
  exchange = NULL;
  mail = NULL;
  password_based = NULL;
  ret = FALSE;

  /* Chain up */
  if (!GOA_PROVIDER_CLASS (goa_exchange_provider_parent_class)->build_object (provider,
                                                                              object,
                                                                              key_file,
                                                                              group,
                                                                              connection,
                                                                              just_added,
                                                                              error))
    goto out;

  password_based = goa_object_get_password_based (GOA_OBJECT (object));
  if (password_based == NULL)
    {
      password_based = goa_password_based_skeleton_new ();
      /* Ensure D-Bus method invocations run in their own thread */
      g_dbus_interface_skeleton_set_flags (G_DBUS_INTERFACE_SKELETON (password_based),
                                           G_DBUS_INTERFACE_SKELETON_FLAGS_HANDLE_METHOD_INVOCATIONS_IN_THREAD);
      goa_object_skeleton_set_password_based (object, password_based);
      g_signal_connect (password_based,
                        "handle-get-password",
                        G_CALLBACK (on_handle_get_password),
                        NULL);
    }

  account = goa_object_get_account (GOA_OBJECT (object));

  /* Email */
  mail = goa_object_get_mail (GOA_OBJECT (object));
  mail_enabled = g_key_file_get_boolean (key_file, group, "MailEnabled", NULL);
  if (mail_enabled)
    {
      if (mail == NULL)
        {
          const gchar *email_address;

          email_address = goa_account_get_presentation_identity (account);
          mail = goa_mail_skeleton_new ();
          g_object_set (G_OBJECT (mail), "email-address", email_address, NULL);
          goa_object_skeleton_set_mail (object, mail);
        }
    }
  else
    {
      if (mail != NULL)
        goa_object_skeleton_set_mail (object, NULL);
    }

  /* Calendar */
  calendar = goa_object_get_calendar (GOA_OBJECT (object));
  calendar_enabled = g_key_file_get_boolean (key_file, group, "CalendarEnabled", NULL);
  if (calendar_enabled)
    {
      if (calendar == NULL)
        {
          calendar = goa_calendar_skeleton_new ();
          goa_object_skeleton_set_calendar (object, calendar);
        }
    }
  else
    {
      if (calendar != NULL)
        goa_object_skeleton_set_calendar (object, NULL);
    }

  /* Contacts */
  contacts = goa_object_get_contacts (GOA_OBJECT (object));
  contacts_enabled = g_key_file_get_boolean (key_file, group, "ContactsEnabled", NULL);
  if (contacts_enabled)
    {
      if (contacts == NULL)
        {
          contacts = goa_contacts_skeleton_new ();
          goa_object_skeleton_set_contacts (object, contacts);
        }
    }
  else
    {
      if (contacts != NULL)
        goa_object_skeleton_set_contacts (object, NULL);
    }

  /* Exchange */
  exchange = goa_object_get_exchange (GOA_OBJECT (object));
  if (exchange == NULL)
    {
      gchar *host;

      host = g_key_file_get_string (key_file, group, "Host", NULL);
      exchange = goa_exchange_skeleton_new ();
      g_object_set (G_OBJECT (exchange), "host", host, NULL);
      goa_object_skeleton_set_exchange (object, exchange);
      g_free (host);
    }

  if (just_added)
    {
      goa_account_set_mail_disabled (account, !mail_enabled);
      goa_account_set_calendar_disabled (account, !calendar_enabled);
      goa_account_set_contacts_disabled (account, !contacts_enabled);

      g_signal_connect (account,
                        "notify::mail-disabled",
                        G_CALLBACK (goa_util_account_notify_property_cb),
                        "MailEnabled");
      g_signal_connect (account,
                        "notify::calendar-disabled",
                        G_CALLBACK (goa_util_account_notify_property_cb),
                        "CalendarEnabled");
      g_signal_connect (account,
                        "notify::contacts-disabled",
                        G_CALLBACK (goa_util_account_notify_property_cb),
                        "ContactsEnabled");
    }

  ret = TRUE;

 out:
  if (exchange != NULL)
    g_object_unref (exchange);
  if (contacts != NULL)
    g_object_unref (contacts);
  if (calendar != NULL)
    g_object_unref (calendar);
  if (mail != NULL)
    g_object_unref (mail);
  if (password_based != NULL)
    g_object_unref (password_based);
  return ret;
}