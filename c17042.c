reexec_in_user_namespace (int ready, char *pause_pid_file_path, char *file_to_read, int outputfd)
{
  int ret;
  pid_t pid;
  char b;
  pid_t ppid = getpid ();
  char **argv;
  char uid[16];
  char gid[16];
  char *listen_fds = NULL;
  char *listen_pid = NULL;
  bool do_socket_activation = false;
  char *cwd = getcwd (NULL, 0);
  sigset_t sigset, oldsigset;

  if (cwd == NULL)
    {
      fprintf (stderr, "error getting current working directory: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }

  listen_pid = getenv("LISTEN_PID");
  listen_fds = getenv("LISTEN_FDS");

  if (listen_pid != NULL && listen_fds != NULL)
    {
      if (strtol(listen_pid, NULL, 10) == getpid())
        do_socket_activation = true;
    }

  sprintf (uid, "%d", geteuid ());
  sprintf (gid, "%d", getegid ());

  pid = syscall_clone (CLONE_NEWUSER|CLONE_NEWNS|SIGCHLD, NULL);
  if (pid < 0)
    {
      fprintf (stderr, "cannot clone: %s\n", strerror (errno));
      check_proc_sys_userns_file (_max_user_namespaces);
      check_proc_sys_userns_file (_unprivileged_user_namespaces);
    }
  if (pid)
    {
      if (do_socket_activation)
        {
          long num_fds;
          num_fds = strtol (listen_fds, NULL, 10);
          if (num_fds != LONG_MIN && num_fds != LONG_MAX)
            {
              long i;
              for (i = 3; i < num_fds + 3; i++)
                if (FD_ISSET (i, &open_files_set))
                  close (i);
            }
          unsetenv ("LISTEN_PID");
          unsetenv ("LISTEN_FDS");
          unsetenv ("LISTEN_FDNAMES");
        }
      return pid;
    }

  if (sigfillset (&sigset) < 0)
    {
      fprintf (stderr, "cannot fill sigset: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }
  if (sigdelset (&sigset, SIGCHLD) < 0)
    {
      fprintf (stderr, "cannot sigdelset(SIGCHLD): %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }
  if (sigdelset (&sigset, SIGTERM) < 0)
    {
      fprintf (stderr, "cannot sigdelset(SIGTERM): %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }
  if (sigprocmask (SIG_BLOCK, &sigset, &oldsigset) < 0)
    {
      fprintf (stderr, "cannot block signals: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }

  argv = get_cmd_line_args (ppid);
  if (argv == NULL)
    {
      fprintf (stderr, "cannot read argv: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }

  if (do_socket_activation)
    {
      char s[32];
      sprintf (s, "%d", getpid());
      setenv ("LISTEN_PID", s, true);
    }

  setenv ("_CONTAINERS_USERNS_CONFIGURED", "init", 1);
  setenv ("_CONTAINERS_ROOTLESS_UID", uid, 1);
  setenv ("_CONTAINERS_ROOTLESS_GID", gid, 1);

  ret = TEMP_FAILURE_RETRY (read (ready, &b, 1));
  if (ret < 0)
    {
      fprintf (stderr, "cannot read from sync pipe: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }
  if (b != '0')
    _exit (EXIT_FAILURE);

  if (syscall_setresgid (0, 0, 0) < 0)
    {
      fprintf (stderr, "cannot setresgid: %s\n", strerror (errno));
      TEMP_FAILURE_RETRY (write (ready, "1", 1));
      _exit (EXIT_FAILURE);
    }

  if (syscall_setresuid (0, 0, 0) < 0)
    {
      fprintf (stderr, "cannot setresuid: %s\n", strerror (errno));
      TEMP_FAILURE_RETRY (write (ready, "1", 1));
      _exit (EXIT_FAILURE);
    }

  if (chdir (cwd) < 0)
    {
      fprintf (stderr, "cannot chdir: %s\n", strerror (errno));
      TEMP_FAILURE_RETRY (write (ready, "1", 1));
      _exit (EXIT_FAILURE);
    }
  free (cwd);

  if (pause_pid_file_path && pause_pid_file_path[0] != '\0')
    {
      if (create_pause_process (pause_pid_file_path, argv) < 0)
        {
          TEMP_FAILURE_RETRY (write (ready, "2", 1));
          _exit (EXIT_FAILURE);
        }
    }

  ret = TEMP_FAILURE_RETRY (write (ready, "0", 1));
  if (ret < 0)
  {
	  fprintf (stderr, "cannot write to ready pipe: %s\n", strerror (errno));
	  _exit (EXIT_FAILURE);
  }
  close (ready);

  if (sigprocmask (SIG_SETMASK, &oldsigset, NULL) < 0)
    {
      fprintf (stderr, "cannot block signals: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }

  if (file_to_read && file_to_read[0])
    {
      ret = copy_file_to_fd (file_to_read, outputfd);
      close (outputfd);
      _exit (ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

  execvp (argv[0], argv);

  _exit (EXIT_FAILURE);
}