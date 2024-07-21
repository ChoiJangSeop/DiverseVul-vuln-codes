reexec_userns_join (int userns, int mountns, char *pause_pid_file_path)
{
  pid_t ppid = getpid ();
  char uid[16];
  char gid[16];
  char **argv;
  int pid;
  char *cwd = getcwd (NULL, 0);
  sigset_t sigset, oldsigset;

  if (cwd == NULL)
    {
      fprintf (stderr, "error getting current working directory: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }

  sprintf (uid, "%d", geteuid ());
  sprintf (gid, "%d", getegid ());

  argv = get_cmd_line_args (ppid);
  if (argv == NULL)
    {
      fprintf (stderr, "cannot read argv: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }

  pid = fork ();
  if (pid < 0)
    fprintf (stderr, "cannot fork: %s\n", strerror (errno));

  if (pid)
    {
      /* We passed down these fds, close them.  */
      int f;
      for (f = 3; f < open_files_max_fd; f++)
        {
          if (FD_ISSET (f, &open_files_set))
            close (f);
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

  setenv ("_CONTAINERS_USERNS_CONFIGURED", "init", 1);
  setenv ("_CONTAINERS_ROOTLESS_UID", uid, 1);
  setenv ("_CONTAINERS_ROOTLESS_GID", gid, 1);

  if (prctl (PR_SET_PDEATHSIG, SIGTERM, 0, 0, 0) < 0)
    {
      fprintf (stderr, "cannot prctl(PR_SET_PDEATHSIG): %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }

  if (setns (userns, 0) < 0)
    {
      fprintf (stderr, "cannot setns: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }
  close (userns);

  if (mountns >= 0 && setns (mountns, 0) < 0)
    {
      fprintf (stderr, "cannot setns: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }
  close (mountns);

  if (syscall_setresgid (0, 0, 0) < 0)
    {
      fprintf (stderr, "cannot setresgid: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }

  if (syscall_setresuid (0, 0, 0) < 0)
    {
      fprintf (stderr, "cannot setresuid: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }

  if (chdir (cwd) < 0)
    {
      fprintf (stderr, "cannot chdir: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }
  free (cwd);

  if (pause_pid_file_path && pause_pid_file_path[0] != '\0')
    {
      /* We ignore errors here as we didn't create the namespace anyway.  */
      create_pause_process (pause_pid_file_path, argv);
    }
  if (sigprocmask (SIG_SETMASK, &oldsigset, NULL) < 0)
    {
      fprintf (stderr, "cannot block signals: %s\n", strerror (errno));
      _exit (EXIT_FAILURE);
    }

  execvp (argv[0], argv);

  _exit (EXIT_FAILURE);
}