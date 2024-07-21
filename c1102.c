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
  GoaFiles *files;
  GoaPasswordBased *password_based;
  SoupURI *uri;
  gboolean calendar_enabled;
  gboolean contacts_enabled;
  gboolean files_enabled;
  gboolean ret;
  const gchar *identity;
  gchar *uri_string;

  account = NULL;
  calendar = NULL;
  contacts = NULL;
  files = NULL;
  password_based = NULL;
  uri = NULL;
  uri_string = NULL;

  ret = FALSE;

  /* Chain up */
  if (!GOA_PROVIDER_CLASS (goa_owncloud_provider_parent_class)->build_object (provider,
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
  identity = goa_account_get_identity (account);
  uri_string = g_key_file_get_string (key_file, group, "Uri", NULL);
  uri = soup_uri_new (uri_string);
  if (uri != NULL)
    soup_uri_set_user (uri, identity);

  /* Calendar */
  calendar = goa_object_get_calendar (GOA_OBJECT (object));
  calendar_enabled = g_key_file_get_boolean (key_file, group, "CalendarEnabled", NULL);
  if (calendar_enabled)
    {
      if (calendar == NULL)
        {
          gchar *uri_caldav;

          uri_caldav = NULL;

          if (uri != NULL)
            {
              gchar *uri_tmp;

              uri_tmp = soup_uri_to_string (uri, FALSE);
              uri_caldav = g_strconcat (uri_tmp, CALDAV_ENDPOINT, NULL);
              g_free (uri_tmp);
            }

          calendar = goa_calendar_skeleton_new ();
          g_object_set (G_OBJECT (calendar), "uri", uri_caldav, NULL);
          goa_object_skeleton_set_calendar (object, calendar);
          g_free (uri_caldav);
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
          gchar *uri_carddav;

          uri_carddav = NULL;

          if (uri != NULL)
            {
              gchar *uri_tmp;

              uri_tmp = soup_uri_to_string (uri, FALSE);
              uri_carddav = g_strconcat (uri_tmp, CARDDAV_ENDPOINT, NULL);
              g_free (uri_tmp);
            }

          contacts = goa_contacts_skeleton_new ();
          g_object_set (G_OBJECT (contacts), "uri", uri_carddav, NULL);
          goa_object_skeleton_set_contacts (object, contacts);
          g_free (uri_carddav);
        }
    }
  else
    {
      if (contacts != NULL)
        goa_object_skeleton_set_contacts (object, NULL);
    }

  /* Files */
  files = goa_object_get_files (GOA_OBJECT (object));
  files_enabled = g_key_file_get_boolean (key_file, group, "FilesEnabled", NULL);
  if (files_enabled)
    {
      if (files == NULL)
        {
          gchar *uri_webdav;

          uri_webdav = NULL;

          if (uri != NULL)
            {
              const gchar *scheme;
              gchar *uri_tmp;

              scheme = soup_uri_get_scheme (uri);
              if (g_strcmp0 (scheme, SOUP_URI_SCHEME_HTTPS) == 0)
                soup_uri_set_scheme (uri, "davs");
              else
                soup_uri_set_scheme (uri, "dav");

              uri_tmp = soup_uri_to_string (uri, FALSE);
              uri_webdav = g_strconcat (uri_tmp, WEBDAV_ENDPOINT, NULL);
              g_free (uri_tmp);
            }

          files = goa_files_skeleton_new ();
          g_object_set (G_OBJECT (files), "uri", uri_webdav, NULL);
          goa_object_skeleton_set_files (object, files);
          g_free (uri_webdav);
        }
    }
  else
    {
      if (files != NULL)
        goa_object_skeleton_set_files (object, NULL);
    }

  if (just_added)
    {
      goa_account_set_calendar_disabled (account, !calendar_enabled);
      goa_account_set_contacts_disabled (account, !contacts_enabled);
      goa_account_set_files_disabled (account, !files_enabled);

      g_signal_connect (account,
                        "notify::calendar-disabled",
                        G_CALLBACK (goa_util_account_notify_property_cb),
                        "CalendarEnabled");
      g_signal_connect (account,
                        "notify::contacts-disabled",
                        G_CALLBACK (goa_util_account_notify_property_cb),
                        "ContactsEnabled");
      g_signal_connect (account,
                        "notify::files-disabled",
                        G_CALLBACK (goa_util_account_notify_property_cb),
                        "FilesEnabled");
    }

  ret = TRUE;

 out:
  g_clear_object (&calendar);
  g_clear_object (&contacts);
  g_clear_object (&files);
  g_clear_object (&password_based);
  g_clear_pointer (&uri, (GDestroyNotify *) soup_uri_free);
  g_free (uri_string);
  return ret;
}