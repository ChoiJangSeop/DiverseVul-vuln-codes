child_setup_func (gpointer user_data)
{
  ChildSetupData *data = (ChildSetupData *) user_data;
  FdMapEntry *fd_map = data->fd_map;
  sigset_t set;
  int i;

  flatpak_close_fds_workaround (3);

  if (data->instance_id_fd != -1)
    drop_cloexec (data->instance_id_fd);

  /* Unblock all signals */
  sigemptyset (&set);
  if (pthread_sigmask (SIG_SETMASK, &set, NULL) == -1)
    {
      g_warning ("Failed to unblock signals when starting child");
      return;
    }

  /* Reset the handlers for all signals to their defaults. */
  for (i = 1; i < NSIG; i++)
    {
      if (i != SIGSTOP && i != SIGKILL)
        signal (i, SIG_DFL);
    }

  for (i = 0; i < data->fd_map_len; i++)
    {
      if (fd_map[i].from != fd_map[i].to)
        {
          dup2 (fd_map[i].from, fd_map[i].to);
          close (fd_map[i].from);
        }
    }

  /* Second pass in case we needed an in-between fd value to avoid conflicts */
  for (i = 0; i < data->fd_map_len; i++)
    {
      if (fd_map[i].to != fd_map[i].final)
        {
          dup2 (fd_map[i].to, fd_map[i].final);
          close (fd_map[i].to);
        }

      /* Ensure we inherit the final fd value */
      drop_cloexec (fd_map[i].final);
    }

  /* We become our own session and process group, because it never makes sense
     to share the flatpak-session-helper dbus activated process group */
  setsid ();
  setpgid (0, 0);

  if (data->set_tty)
    {
      /* data->tty is our from fd which is closed at this point.
       * so locate the destination fd and use it for the ioctl.
       */
      for (i = 0; i < data->fd_map_len; i++)
        {
          if (fd_map[i].from == data->tty)
            {
              if (ioctl (fd_map[i].final, TIOCSCTTY, 0) == -1)
                g_debug ("ioctl(%d, TIOCSCTTY, 0) failed: %s",
                         fd_map[i].final, strerror (errno));
              break;
            }
        }
    }
}