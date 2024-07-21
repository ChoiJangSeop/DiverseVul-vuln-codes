gdm_session_worker_start_user_session (GdmSessionWorker  *worker,
                                       GError           **error)
{
        struct passwd *passwd_entry;
        pid_t session_pid;
        int   error_code;

        g_debug ("GdmSessionWorker: querying pam for user environment");
        gdm_session_worker_update_environment_from_pam (worker);

        register_ck_session (worker);

        passwd_entry = getpwnam (worker->priv->username);

#ifdef  HAVE_LOGINDEVPERM
        /*
         * Only do logindevperm processing if /dev/console or
         * a device associated with a VT
         */
        if (worker->priv->display_device != NULL &&
           (strncmp (worker->priv->display_device, "/dev/vt/", strlen ("/dev/vt/")) == 0 ||
            strcmp  (worker->priv->display_device, "/dev/console") == 0)) {
                g_debug ("Logindevperm login for user %s, device %s",
                         worker->priv->username,
                         worker->priv->display_device);
                (void) di_devperm_login (worker->priv->display_device,
                                         passwd_entry->pw_uid,
                                         passwd_entry->pw_gid,
                                         NULL);
        }
#endif  /* HAVE_LOGINDEVPERM */

        g_debug ("GdmSessionWorker: opening user session with program '%s'",
                 worker->priv->arguments[0]);

        error_code = PAM_SUCCESS;

        session_pid = fork ();

        if (session_pid < 0) {
                g_set_error (error,
                             GDM_SESSION_WORKER_ERROR,
                             GDM_SESSION_WORKER_ERROR_OPENING_SESSION,
                             "%s", g_strerror (errno));
                error_code = PAM_ABORT;
                goto out;
        }

        if (session_pid == 0) {
                char **environment;
                char  *home_dir;
                int    fd;

                if (setuid (worker->priv->uid) < 0) {
                        g_debug ("GdmSessionWorker: could not reset uid - %s", g_strerror (errno));
                        _exit (1);
                }

                if (setsid () < 0) {
                        g_debug ("GdmSessionWorker: could not set pid '%u' as leader of new session and process group - %s",
                                 (guint) getpid (), g_strerror (errno));
                        _exit (2);
                }

                environment = gdm_session_worker_get_environment (worker);

                g_assert (geteuid () == getuid ());

                home_dir = g_hash_table_lookup (worker->priv->environment,
                                                "HOME");

                if ((home_dir == NULL) || g_chdir (home_dir) < 0) {
                        g_chdir ("/");
                }

                fd = open ("/dev/null", O_RDWR);
                dup2 (fd, STDIN_FILENO);
                close (fd);

                fd = _open_session_log (home_dir);
                dup2 (fd, STDOUT_FILENO);
                dup2 (fd, STDERR_FILENO);
                close (fd);

                _save_user_settings (worker, home_dir);

                gdm_session_execute (worker->priv->arguments[0],
                                     worker->priv->arguments,
                                     environment,
                                     TRUE);

                g_debug ("GdmSessionWorker: child '%s' could not be started - %s",
                         worker->priv->arguments[0],
                         g_strerror (errno));
                g_strfreev (environment);

                _exit (127);
        }

        worker->priv->child_pid = session_pid;

        g_debug ("GdmSessionWorker: session opened creating reply...");
        g_assert (sizeof (GPid) <= sizeof (int));

        g_debug ("GdmSessionWorker: state SESSION_STARTED");
        worker->priv->state = GDM_SESSION_WORKER_STATE_SESSION_STARTED;

        gdm_session_worker_watch_child (worker);

 out:
        if (error_code != PAM_SUCCESS) {
                gdm_session_worker_uninitialize_pam (worker, error_code);
                return FALSE;
        }

        return TRUE;
}