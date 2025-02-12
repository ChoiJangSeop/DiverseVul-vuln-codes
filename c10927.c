main (int argc, char *argv[])
{
  guint n;
  guint ret;
  gint rc;
  gboolean opt_show_help;
  gboolean opt_show_version;
  gboolean opt_disable_internal_agent;
  PolkitAuthority *authority;
  PolkitAuthorizationResult *result;
  PolkitSubject *subject;
  PolkitDetails *details;
  GError *error;
  gchar *action_id;
  gboolean allow_gui;
  gchar **exec_argv;
  gchar *path;
  struct passwd pwstruct;
  gchar pwbuf[8192];
  gchar *s;
  const gchar *environment_variables_to_save[] = {
    "SHELL",
    "LANG",
    "LINGUAS",
    "LANGUAGE",
    "LC_COLLATE",
    "LC_CTYPE",
    "LC_MESSAGES",
    "LC_MONETARY",
    "LC_NUMERIC",
    "LC_TIME",
    "LC_ALL",
    "TERM",
    "COLORTERM",

    /* By default we don't allow running X11 apps, as it does not work in the
     * general case. See
     *
     *  https://bugs.freedesktop.org/show_bug.cgi?id=17970#c26
     *
     * and surrounding comments for a lot of discussion about this.
     *
     * However, it can be enabled for some selected and tested legacy programs
     * which previously used e. g. gksu, by setting the
     * org.freedesktop.policykit.exec.allow_gui annotation to a nonempty value.
     * See https://bugs.freedesktop.org/show_bug.cgi?id=38769 for details.
     */
    "DISPLAY",
    "XAUTHORITY",
    NULL
  };
  GPtrArray *saved_env;
  gchar *opt_user;
  pid_t pid_of_caller;
  gpointer local_agent_handle;

  ret = 127;
  authority = NULL;
  subject = NULL;
  details = NULL;
  result = NULL;
  action_id = NULL;
  saved_env = NULL;
  path = NULL;
  exec_argv = NULL;
  command_line = NULL;
  opt_user = NULL;
  local_agent_handle = NULL;

  /* Disable remote file access from GIO. */
  setenv ("GIO_USE_VFS", "local", 1);

  /* check for correct invocation */
  if (geteuid () != 0)
    {
      g_printerr ("pkexec must be setuid root\n");
      goto out;
    }

  original_user_name = g_strdup (g_get_user_name ());
  if (original_user_name == NULL)
    {
      g_printerr ("Error getting user name.\n");
      goto out;
    }

  if ((original_cwd = g_get_current_dir ()) == NULL)
    {
      g_printerr ("Error getting cwd: %s\n",
                  g_strerror (errno));
      goto out;
    }

  /* First process options and find the command-line to invoke. Avoid using fancy library routines
   * that depend on environtment variables since we haven't cleared the environment just yet.
   */
  opt_show_help = FALSE;
  opt_show_version = FALSE;
  opt_disable_internal_agent = FALSE;
  for (n = 1; n < (guint) argc; n++)
    {
      if (strcmp (argv[n], "--help") == 0)
        {
          opt_show_help = TRUE;
        }
      else if (strcmp (argv[n], "--version") == 0)
        {
          opt_show_version = TRUE;
        }
      else if (strcmp (argv[n], "--user") == 0 || strcmp (argv[n], "-u") == 0)
        {
          n++;
          if (n >= (guint) argc)
            {
              usage (argc, argv);
              goto out;
            }

          if (opt_user != NULL)
            {
              g_printerr ("--user specified twice\n");
              goto out;
            }
          opt_user = g_strdup (argv[n]);
        }
      else if (strcmp (argv[n], "--disable-internal-agent") == 0)
        {
          opt_disable_internal_agent = TRUE;
        }
      else
        {
          break;
        }
    }

  if (opt_show_help)
    {
      usage (argc, argv);
      ret = 0;
      goto out;
    }
  else if (opt_show_version)
    {
      g_print ("pkexec version %s\n", PACKAGE_VERSION);
      ret = 0;
      goto out;
    }

  if (opt_user == NULL)
    opt_user = g_strdup ("root");

  /* Look up information about the user we care about - yes, the return
   * value of this function is a bit funky
   */
  rc = getpwnam_r (opt_user, &pwstruct, pwbuf, sizeof pwbuf, &pw);
  if (rc == 0 && pw == NULL)
    {
      g_printerr ("User `%s' does not exist.\n", opt_user);
      goto out;
    }
  else if (pw == NULL)
    {
      g_printerr ("Error getting information for user `%s': %s\n", opt_user, g_strerror (rc));
      goto out;
    }

  /* Now figure out the command-line to run - argv is guaranteed to be NULL-terminated, see
   *
   *  http://lkml.indiana.edu/hypermail/linux/kernel/0409.2/0287.html
   *
   * but do check this is the case.
   *
   * We also try to locate the program in the path if a non-absolute path is given.
   */
  g_assert (argv[argc] == NULL);
  path = g_strdup (argv[n]);
  if (path == NULL)
    {
      GPtrArray *shell_argv;

      path = g_strdup (pwstruct.pw_shell);
      if (!path)
	{
          g_printerr ("No shell configured or error retrieving pw_shell\n");
          goto out;
	}
      /* If you change this, be sure to change the if (!command_line)
	 case below too */
      command_line = g_strdup (path);
      shell_argv = g_ptr_array_new ();
      g_ptr_array_add (shell_argv, path);
      g_ptr_array_add (shell_argv, NULL);
      exec_argv = (char**)g_ptr_array_free (shell_argv, FALSE);
    }
  if (path[0] != '/')
    {
      /* g_find_program_in_path() is not suspectible to attacks via the environment */
      s = g_find_program_in_path (path);
      if (s == NULL)
        {
          g_printerr ("Cannot run program %s: %s\n", path, strerror (ENOENT));
          goto out;
        }
      g_free (path);
      argv[n] = path = s;
    }
  if (access (path, F_OK) != 0)
    {
      g_printerr ("Error accessing %s: %s\n", path, g_strerror (errno));
      goto out;
    }

  if (!command_line)
    {
      /* If you change this, be sure to change the path == NULL case
	 above too */
      command_line = g_strjoinv (" ", argv + n);
      exec_argv = argv + n;
    }

  /* now save the environment variables we care about */
  saved_env = g_ptr_array_new ();
  for (n = 0; environment_variables_to_save[n] != NULL; n++)
    {
      const gchar *key = environment_variables_to_save[n];
      const gchar *value;

      value = g_getenv (key);
      if (value == NULL)
        continue;

      /* To qualify for the paranoia goldstar - we validate the value of each
       * environment variable passed through - this is to attempt to avoid
       * exploits in (potentially broken) programs launched via pkexec(1).
       */
      if (!validate_environment_variable (key, value))
        goto out;

      g_ptr_array_add (saved_env, g_strdup (key));
      g_ptr_array_add (saved_env, g_strdup (value));
    }

  /* $XAUTHORITY is "special" - if unset, we need to set it to ~/.Xauthority. Yes,
   * this is broken but it's unfortunately how things work (see fdo #51623 for
   * details)
   */
  if (g_getenv ("XAUTHORITY") == NULL)
    {
      const gchar *home;

      /* pre-2.36 GLib does not examine $HOME (it always looks in /etc/passwd) and
       * this is not what we want
       */
      home = g_getenv ("HOME");
      if (home == NULL)
        home = g_get_home_dir ();

      if (home != NULL)
        {
          g_ptr_array_add (saved_env, g_strdup ("XAUTHORITY"));
          g_ptr_array_add (saved_env, g_build_filename (home, ".Xauthority", NULL));
        }
    }

  /* Nuke the environment to get a well-known and sanitized environment to avoid attacks
   * via e.g. the DBUS_SYSTEM_BUS_ADDRESS environment variable and similar.
   */
  if (clearenv () != 0)
    {
      g_printerr ("Error clearing environment: %s\n", g_strerror (errno));
      goto out;
    }

  /* make sure we are nuked if the parent process dies */
#ifdef __linux__
  if (prctl (PR_SET_PDEATHSIG, SIGTERM) != 0)
    {
      g_printerr ("prctl(PR_SET_PDEATHSIG, SIGTERM) failed: %s\n", g_strerror (errno));
      goto out;
    }
#else
#warning "Please add OS specific code to catch when the parent dies"
#endif

  /* Figure out the parent process */
  pid_of_caller = getppid ();
  if (pid_of_caller == 1)
    {
      /* getppid() can return 1 if the parent died (meaning that we are reaped
       * by /sbin/init); In that case we simpy bail.
       */
      g_printerr ("Refusing to render service to dead parents.\n");
      goto out;
    }

  /* This process we want to check an authorization for is the process
   * that launched us - our parent process.
   *
   * At the time the parent process fork()'ed and exec()'ed us, the
   * process had the same real-uid that we have now. So we use this
   * real-uid instead of of looking it up to avoid TOCTTOU issues
   * (consider the parent process exec()'ing a setuid helper).
   *
   * On the other hand, the monotonic process start-time is guaranteed
   * to never change so it's safe to look that up given only the PID
   * since we are guaranteed to be nuked if the parent goes away
   * (cf. the prctl(2) call above).
   */
  subject = polkit_unix_process_new_for_owner (pid_of_caller,
                                               0, /* 0 means "look up start-time in /proc" */
                                               getuid ());
  /* really double-check the invariants guaranteed by the PolkitUnixProcess class */
  g_assert (subject != NULL);
  g_assert (polkit_unix_process_get_pid (POLKIT_UNIX_PROCESS (subject)) == pid_of_caller);
  g_assert (polkit_unix_process_get_uid (POLKIT_UNIX_PROCESS (subject)) >= 0);
  g_assert (polkit_unix_process_get_start_time (POLKIT_UNIX_PROCESS (subject)) > 0);

  error = NULL;
  authority = polkit_authority_get_sync (NULL /* GCancellable* */, &error);
  if (authority == NULL)
    {
      g_printerr ("Error getting authority: %s\n", error->message);
      g_error_free (error);
      goto out;
    }

  g_assert (path != NULL);
  g_assert (exec_argv != NULL);
  action_id = find_action_for_path (authority,
                                    path,
                                    exec_argv[1],
                                    &allow_gui);
  g_assert (action_id != NULL);

  details = polkit_details_new ();
  polkit_details_insert (details, "user", pw->pw_name);
  if (pw->pw_gecos != NULL)
    polkit_details_insert (details, "user.gecos", pw->pw_gecos);
  if (pw->pw_gecos != NULL && strlen (pw->pw_gecos) > 0)
    s = g_strdup_printf ("%s (%s)", pw->pw_gecos, pw->pw_name);
  else
    s = g_strdup_printf ("%s", pw->pw_name);
  polkit_details_insert (details, "user.display", s);
  g_free (s);
  polkit_details_insert (details, "program", path);
  polkit_details_insert (details, "command_line", command_line);
  if (g_strcmp0 (action_id, "org.freedesktop.policykit.exec") == 0)
    {
      if (pw->pw_uid == 0)
        {
          polkit_details_insert (details, "polkit.message",
                                 /* Translators: message shown when trying to run a program as root. Do not
                                  * translate the $(program) fragment - it will be expanded to the path
                                  * of the program e.g.  /bin/bash.
                                  */
                                 N_("Authentication is needed to run `$(program)' as the super user"));
        }
      else
        {
          polkit_details_insert (details, "polkit.message",
                                 /* Translators: message shown when trying to run a program as another user.
                                  * Do not translate the $(program) or $(user) fragments - the former will
                                  * be expanded to the path of the program e.g. "/bin/bash" and the latter
                                  * to the user e.g. "John Doe (johndoe)" or "johndoe".
                                  */
                                 N_("Authentication is needed to run `$(program)' as user $(user.display)"));
        }
    }
  polkit_details_insert (details, "polkit.gettext_domain", GETTEXT_PACKAGE);

 try_again:
  error = NULL;
  result = polkit_authority_check_authorization_sync (authority,
                                                      subject,
                                                      action_id,
                                                      details,
                                                      POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION,
                                                      NULL,
                                                      &error);
  if (result == NULL)
    {
      g_printerr ("Error checking for authorization %s: %s\n",
                  action_id,
                  error->message);
      goto out;
    }

  if (polkit_authorization_result_get_is_authorized (result))
    {
      /* do nothing */
    }
  else if (polkit_authorization_result_get_is_challenge (result))
    {
      if (local_agent_handle == NULL && !opt_disable_internal_agent)
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
          g_printerr ("Error executing command as another user: No authentication agent found.\n");
          goto out;
        }
    }
  else
    {
      if (polkit_authorization_result_get_dismissed (result))
        {
          log_message (LOG_WARNING, TRUE,
                       "Error executing command as another user: Request dismissed");
          ret = 126;
        }
      else
        {
          log_message (LOG_WARNING, TRUE,
                       "Error executing command as another user: Not authorized");
          g_printerr ("\n"
                      "This incident has been reported.\n");
        }
      goto out;
    }

  /* Set PATH to a safe list */
  g_ptr_array_add (saved_env, g_strdup ("PATH"));
  if (pw->pw_uid != 0)
    s = g_strdup_printf ("/usr/bin:/bin:/usr/sbin:/sbin:%s/bin", pw->pw_dir);
  else
    s = g_strdup_printf ("/usr/sbin:/usr/bin:/sbin:/bin:%s/bin", pw->pw_dir);
  g_ptr_array_add (saved_env, s);
  g_ptr_array_add (saved_env, g_strdup ("LOGNAME"));
  g_ptr_array_add (saved_env, g_strdup (pw->pw_name));
  g_ptr_array_add (saved_env, g_strdup ("USER"));
  g_ptr_array_add (saved_env, g_strdup (pw->pw_name));
  g_ptr_array_add (saved_env, g_strdup ("HOME"));
  g_ptr_array_add (saved_env, g_strdup (pw->pw_dir));

  s = g_strdup_printf ("%d", getuid ());
  g_ptr_array_add (saved_env, g_strdup ("PKEXEC_UID"));
  g_ptr_array_add (saved_env, s);

  /* set the environment */
  for (n = 0; n < saved_env->len - 1; n += 2)
    {
      const gchar *key = saved_env->pdata[n];
      const gchar *value = saved_env->pdata[n + 1];

      /* Only set $DISPLAY and $XAUTHORITY when explicitly allowed in the .policy */
      if (!allow_gui &&
              (strcmp (key, "DISPLAY") == 0 || strcmp (key, "XAUTHORITY") == 0))
          continue;

      if (!g_setenv (key, value, TRUE))
        {
          g_printerr ("Error setting environment variable %s to '%s': %s\n",
                      key,
                      value,
                      g_strerror (errno));
          goto out;
        }
    }

  /* set close_on_exec on all file descriptors except stdin, stdout, stderr */
  if (!fdwalk (set_close_on_exec, GINT_TO_POINTER (3)))
    {
      g_printerr ("Error setting close-on-exec for file desriptors\n");
      goto out;
    }

  /* if not changing to uid 0, become uid 0 before changing to the user */
  if (pw->pw_uid != 0)
    {
      setreuid (0, 0);
      if ((geteuid () != 0) || (getuid () != 0))
        {
          g_printerr ("Error becoming uid 0: %s\n", g_strerror (errno));
          goto out;
        }
    }

  /* open session - with PAM enabled, this runs the open_session() part of the PAM
   * stack - this includes applying limits via pam_limits.so but also other things
   * requested via the current PAM configuration.
   *
   * NOTE NOTE NOTE: pam_limits.so doesn't seem to clear existing limits - e.g.
   *
   *  $ ulimit -t
   *  unlimited
   *
   *  $ su -
   *  Password:
   *  # ulimit -t
   *  unlimited
   *  # logout
   *
   *  $ ulimit -t 1000
   *  $ ulimit -t
   *  1000
   *  $ su -
   *  Password:
   *  # ulimit -t
   *  1000
   *
   * TODO: The question here is whether we should clear the limits before applying them?
   * As evident above, neither su(1) (and, for that matter, nor sudo(8)) does this.
   */
#ifdef POLKIT_AUTHFW_PAM
  if (!open_session (pw->pw_name,
		     pw->pw_uid))
    {
      goto out;
    }
#endif /* POLKIT_AUTHFW_PAM */

  /* become the user */
  if (setgroups (0, NULL) != 0)
    {
      g_printerr ("Error setting groups: %s\n", g_strerror (errno));
      goto out;
    }
  if (initgroups (pw->pw_name, pw->pw_gid) != 0)
    {
      g_printerr ("Error initializing groups for %s: %s\n", pw->pw_name, g_strerror (errno));
      goto out;
    }
  setregid (pw->pw_gid, pw->pw_gid);
  setreuid (pw->pw_uid, pw->pw_uid);
  if ((geteuid () != pw->pw_uid) || (getuid () != pw->pw_uid) ||
      (getegid () != pw->pw_gid) || (getgid () != pw->pw_gid))
    {
      g_printerr ("Error becoming real+effective uid %d and gid %d: %s\n", pw->pw_uid, pw->pw_gid, g_strerror (errno));
      goto out;
    }

  /* change to home directory */
  if (chdir (pw->pw_dir) != 0)
    {
      g_printerr ("Error changing to home directory %s: %s\n", pw->pw_dir, g_strerror (errno));
      goto out;
    }

  /* Log the fact that we're executing a command */
  log_message (LOG_NOTICE, FALSE, "Executing command");

  /* exec the program */
  if (execv (path, exec_argv) != 0)
    {
      g_printerr ("Error executing %s: %s\n", path, g_strerror (errno));
      goto out;
    }

  /* if exec doesn't fail, it never returns... */
  g_assert_not_reached ();

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

  if (saved_env != NULL)
    {
      g_ptr_array_foreach (saved_env, (GFunc) g_free, NULL);
      g_ptr_array_free (saved_env, TRUE);
    }

  g_free (original_cwd);
  g_free (path);
  g_free (command_line);
  g_free (opt_user);
  g_free (original_user_name);

  return ret;
}