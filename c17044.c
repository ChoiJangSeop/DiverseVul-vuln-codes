static void __attribute__((constructor)) init()
{
  const char *xdg_runtime_dir;
  const char *pause;
  DIR *d;

  pause = getenv ("_PODMAN_PAUSE");
  if (pause && pause[0])
    {
      do_pause ();
      _exit (EXIT_FAILURE);
    }

  /* Store how many FDs were open before the Go runtime kicked in.  */
  d = opendir ("/proc/self/fd");
  if (d)
    {
      struct dirent *ent;

      FD_ZERO (&open_files_set);
      for (ent = readdir (d); ent; ent = readdir (d))
        {
          int fd = atoi (ent->d_name);
          if (fd != dirfd (d))
            {
              if (fd > open_files_max_fd)
                open_files_max_fd = fd;
              FD_SET (fd, &open_files_set);
            }
        }
      closedir (d);
    }

  /* Shortcut.  If we are able to join the pause pid file, do it now so we don't
     need to re-exec.  */
  xdg_runtime_dir = getenv ("XDG_RUNTIME_DIR");
  if (geteuid () != 0 && xdg_runtime_dir && xdg_runtime_dir[0] && can_use_shortcut ())
    {
      int r;
      int fd;
      long pid;
      char buf[12];
      uid_t uid;
      gid_t gid;
      char path[PATH_MAX];
      const char *const suffix = "/libpod/pause.pid";
      char *cwd = getcwd (NULL, 0);
      char uid_fmt[16];
      char gid_fmt[16];

      if (cwd == NULL)
        {
          fprintf (stderr, "error getting current working directory: %s\n", strerror (errno));
          _exit (EXIT_FAILURE);
        }

      if (strlen (xdg_runtime_dir) >= PATH_MAX - strlen (suffix))
        {
          fprintf (stderr, "invalid value for XDG_RUNTIME_DIR: %s", strerror (ENAMETOOLONG));
          exit (EXIT_FAILURE);
        }

      sprintf (path, "%s%s", xdg_runtime_dir, suffix);
      fd = open (path, O_RDONLY);
      if (fd < 0)
        {
          free (cwd);
          return;
        }

      r = TEMP_FAILURE_RETRY (read (fd, buf, sizeof (buf) - 1));
      close (fd);
      if (r < 0)
        {
          free (cwd);
          return;
        }
      buf[r] = '\0';

      pid = strtol (buf, NULL, 10);
      if (pid == LONG_MAX)
        {
          free (cwd);
          return;
        }

      uid = geteuid ();
      gid = getegid ();

      sprintf (path, "/proc/%ld/ns/user", pid);
      fd = open (path, O_RDONLY);
      if (fd < 0 || setns (fd, 0) < 0)
        {
          free (cwd);
          return;
        }
      close (fd);

      /* Errors here cannot be ignored as we already joined a ns.  */
      sprintf (path, "/proc/%ld/ns/mnt", pid);
      fd = open (path, O_RDONLY);
      if (fd < 0)
        {
          fprintf (stderr, "cannot open %s: %s", path, strerror (errno));
          exit (EXIT_FAILURE);
        }

      sprintf (uid_fmt, "%d", uid);
      sprintf (gid_fmt, "%d", gid);

      setenv ("_CONTAINERS_USERNS_CONFIGURED", "init", 1);
      setenv ("_CONTAINERS_ROOTLESS_UID", uid_fmt, 1);
      setenv ("_CONTAINERS_ROOTLESS_GID", gid_fmt, 1);

      r = setns (fd, 0);
      if (r < 0)
        {
          fprintf (stderr, "cannot join mount namespace for %ld: %s", pid, strerror (errno));
          exit (EXIT_FAILURE);
        }
      close (fd);

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
      rootless_uid_init = uid;
      rootless_gid_init = gid;
    }
}