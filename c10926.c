main (int argc, char *argv[])
{
  guint n;
  guint ret;
  gchar *action_id;
  gboolean opt_show_help;
  gboolean opt_show_version;
  gboolean allow_user_interaction;
  gboolean enable_internal_agent;
  gboolean list_temp;
  gboolean revoke_temp;
  PolkitAuthority *authority;
  PolkitAuthorizationResult *result;
  PolkitSubject *subject;
  PolkitDetails *details;
  PolkitCheckAuthorizationFlags flags;
  PolkitDetails *result_details;
  GError *error;
  gpointer local_agent_handle;

  subject = NULL;
  action_id = NULL;
  details = NULL;
  authority = NULL;
  result = NULL;
  allow_user_interaction = FALSE;
  enable_internal_agent = FALSE;
  list_temp = FALSE;
  revoke_temp = FALSE;
  local_agent_handle = NULL;
  ret = 126;

  /* Disable remote file access from GIO. */
  setenv ("GIO_USE_VFS", "local", 1);

  details = polkit_details_new ();

  opt_show_help = FALSE;
  opt_show_version = FALSE;
  g_set_prgname ("pkcheck");
  for (n = 1; n < (guint) argc; n++)
    {
      if (g_strcmp0 (argv[n], "--help") == 0)
        {
          opt_show_help = TRUE;
        }
      else if (g_strcmp0 (argv[n], "--version") == 0)
        {
          opt_show_version = TRUE;
        }
      else if (g_strcmp0 (argv[n], "--process") == 0 || g_strcmp0 (argv[n], "-p") == 0)
        {
          gint pid;
	  guint uid;
          guint64 pid_start_time;

          n++;
          if (n >= (guint) argc)
            {
	      g_printerr (_("%s: Argument expected after `%s'\n"),
			  g_get_prgname (), "--process");
              goto out;
            }

          if (sscanf (argv[n], "%i,%" G_GUINT64_FORMAT ",%u", &pid, &pid_start_time, &uid) == 3)
            {
              subject = polkit_unix_process_new_for_owner (pid, pid_start_time, uid);
            }
          else if (sscanf (argv[n], "%i,%" G_GUINT64_FORMAT, &pid, &pid_start_time) == 2)
            {
	      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
              subject = polkit_unix_process_new_full (pid, pid_start_time);
	      G_GNUC_END_IGNORE_DEPRECATIONS
            }
          else if (sscanf (argv[n], "%i", &pid) == 1)
            {
	      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
              subject = polkit_unix_process_new (pid);
	      G_GNUC_END_IGNORE_DEPRECATIONS
            }
          else
            {
	      g_printerr (_("%s: Invalid --process value `%s'\n"),
			  g_get_prgname (), argv[n]);
              goto out;
            }
        }
      else if (g_strcmp0 (argv[n], "--system-bus-name") == 0 || g_strcmp0 (argv[n], "-s") == 0)
        {
          n++;
          if (n >= (guint) argc)
            {
	      g_printerr (_("%s: Argument expected after `%s'\n"),
			  g_get_prgname (), "--system-bus-name");
              goto out;
            }

          subject = polkit_system_bus_name_new (argv[n]);
        }
      else if (g_strcmp0 (argv[n], "--action-id") == 0 || g_strcmp0 (argv[n], "-a") == 0)
        {
          n++;
          if (n >= (guint) argc)
            {
	      g_printerr (_("%s: Argument expected after `%s'\n"),
			  g_get_prgname (), "--action-id");
              goto out;
            }

          action_id = g_strdup (argv[n]);
        }
      else if (g_strcmp0 (argv[n], "--detail") == 0 || g_strcmp0 (argv[n], "-d") == 0)
        {
          const gchar *key;
          const gchar *value;

          n++;
          if (n >= (guint) argc)
            {
	      g_printerr (_("%s: Two arguments expected after `--detail'\n"),
			  g_get_prgname ());
              goto out;
            }
          key = argv[n];

          n++;
          if (n >= (guint) argc)
            {
	      g_printerr (_("%s: Two arguments expected after `--detail'\n"),
			  g_get_prgname ());
              goto out;
            }
          value = argv[n];

          polkit_details_insert (details, key, value);
        }
      else if (g_strcmp0 (argv[n], "--allow-user-interaction") == 0 || g_strcmp0 (argv[n], "-u") == 0)
        {
          allow_user_interaction = TRUE;
        }
      else if (g_strcmp0 (argv[n], "--enable-internal-agent") == 0)
        {
          enable_internal_agent = TRUE;
        }
      else if (g_strcmp0 (argv[n], "--list-temp") == 0)
        {
          list_temp = TRUE;
        }
      else if (g_strcmp0 (argv[n], "--revoke-temp") == 0)
        {
          revoke_temp = TRUE;
        }
      else
        {
          break;
        }
    }
  if (argv[n] != NULL)
    {
      g_printerr (_("%s: Unexpected argument `%s'\n"), g_get_prgname (),
		  argv[n]);
      goto out;
    }

  if (opt_show_help)
    {
      help ();
      ret = 0;
      goto out;
    }
  else if (opt_show_version)
    {
      g_print ("pkcheck version %s\n", PACKAGE_VERSION);
      ret = 0;
      goto out;
    }

  if (list_temp)
    {
      ret = do_list_or_revoke_temp_authz (FALSE);
      goto out;
    }
  else if (revoke_temp)
    {
      ret = do_list_or_revoke_temp_authz (TRUE);
      goto out;
    }
  else if (subject == NULL)
    {
      g_printerr (_("%s: Subject not specified\n"), g_get_prgname ());
      goto out;
    }

  error = NULL;
  authority = polkit_authority_get_sync (NULL /* GCancellable* */, &error);
  if (authority == NULL)
    {
      g_printerr ("Error getting authority: %s\n", error->message);
      g_error_free (error);
      goto out;
    }

 try_again:
  error = NULL;
  flags = POLKIT_CHECK_AUTHORIZATION_FLAGS_NONE;
  if (allow_user_interaction)
    flags |= POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION;
  result = polkit_authority_check_authorization_sync (authority,
                                                      subject,
                                                      action_id,
                                                      details,
                                                      flags,
                                                      NULL,
                                                      &error);
  if (result == NULL)
    {
      g_printerr ("Error checking for authorization %s: %s\n",
                  action_id,
                  error->message);
      ret = 127;
      goto out;
    }

  result_details = polkit_authorization_result_get_details (result);
  if (result_details != NULL)
    {
      gchar **keys;

      keys = polkit_details_get_keys (result_details);
      for (n = 0; keys != NULL && keys[n] != NULL; n++)
        {
          const gchar *key;
          const gchar *value;
          gchar *s;

          key = keys[n];
          value = polkit_details_lookup (result_details, key);

          s = escape_str (key);
          g_print ("%s", s);
          g_free (s);
          g_print ("=");
          s = escape_str (value);
          g_print ("%s", s);
          g_free (s);
          g_print ("\n");
        }

      g_strfreev (keys);
    }

  if (polkit_authorization_result_get_is_authorized (result))
    {
      ret = 0;
    }
  else if (polkit_authorization_result_get_is_challenge (result))
    {
      if (allow_user_interaction)
        {
          if (local_agent_handle == NULL && enable_internal_agent)
            {
              PolkitAgentListener *listener;
              error = NULL;
              /* this will fail if we can't find a controlling terminal */
              listener = polkit_agent_text_listener_new (NULL, &error);
              if (listener == NULL)
                {
                  g_printerr ("Error creating textual authentication agent: %s\n", error->message);
                  g_error_free (error);
                  goto out;
                }
              local_agent_handle = polkit_agent_listener_register (listener,
                                                                   POLKIT_AGENT_REGISTER_FLAGS_RUN_IN_THREAD,
                                                                   subject,
                                                                   NULL, /* object_path */
                                                                   NULL, /* GCancellable */
                                                                   &error);
              g_object_unref (listener);
              if (local_agent_handle == NULL)
                {
                  g_printerr ("Error registering local authentication agent: %s\n", error->message);
                  g_error_free (error);
                  goto out;
                }
              g_object_unref (result);
              result = NULL;
              goto try_again;
            }
          else
            {
              g_printerr ("Authorization requires authentication but no agent is available.\n");
            }
        }
      else
        {
          g_printerr ("Authorization requires authentication and -u wasn't passed.\n");
        }
      ret = 2;
    }
  else if (polkit_authorization_result_get_dismissed (result))
    {
      g_printerr ("Authentication request was dismissed.\n");
      ret = 3;
    }
  else
    {
      g_printerr ("Not authorized.\n");
      ret = 1;
    }

 out:
  /* if applicable, nuke the local authentication agent */
  if (local_agent_handle != NULL)
    polkit_agent_listener_unregister (local_agent_handle);

  if (result != NULL)
    g_object_unref (result);

  g_free (action_id);

  if (details != NULL)
    g_object_unref (details);

  if (subject != NULL)
    g_object_unref (subject);

  if (authority != NULL)
    g_object_unref (authority);

  return ret;
}