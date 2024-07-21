flatpak_run_add_environment_args (FlatpakBwrap    *bwrap,
                                  const char      *app_info_path,
                                  FlatpakRunFlags  flags,
                                  const char      *app_id,
                                  FlatpakContext  *context,
                                  GFile           *app_id_dir,
                                  GPtrArray       *previous_app_id_dirs,
                                  FlatpakExports **exports_out,
                                  GCancellable    *cancellable,
                                  GError         **error)
{
  g_autoptr(GError) my_error = NULL;
  g_autoptr(FlatpakExports) exports = NULL;
  g_autoptr(FlatpakBwrap) proxy_arg_bwrap = flatpak_bwrap_new (flatpak_bwrap_empty_env);
  gboolean has_wayland = FALSE;
  gboolean allow_x11 = FALSE;

  if ((context->shares & FLATPAK_CONTEXT_SHARED_IPC) == 0)
    {
      g_debug ("Disallowing ipc access");
      flatpak_bwrap_add_args (bwrap, "--unshare-ipc", NULL);
    }

  if ((context->shares & FLATPAK_CONTEXT_SHARED_NETWORK) == 0)
    {
      g_debug ("Disallowing network access");
      flatpak_bwrap_add_args (bwrap, "--unshare-net", NULL);
    }

  if (context->devices & FLATPAK_CONTEXT_DEVICE_ALL)
    {
      flatpak_bwrap_add_args (bwrap,
                              "--dev-bind", "/dev", "/dev",
                              NULL);
      /* Don't expose the host /dev/shm, just the device nodes, unless explicitly allowed */
      if (g_file_test ("/dev/shm", G_FILE_TEST_IS_DIR))
        {
          if ((context->devices & FLATPAK_CONTEXT_DEVICE_SHM) == 0)
            flatpak_bwrap_add_args (bwrap,
                                    "--tmpfs", "/dev/shm",
                                    NULL);
        }
      else if (g_file_test ("/dev/shm", G_FILE_TEST_IS_SYMLINK))
        {
          g_autofree char *link = flatpak_readlink ("/dev/shm", NULL);

          /* On debian (with sysv init) the host /dev/shm is a symlink to /run/shm, so we can't
             mount on top of it. */
          if (g_strcmp0 (link, "/run/shm") == 0)
            {
              if (context->devices & FLATPAK_CONTEXT_DEVICE_SHM &&
                  g_file_test ("/run/shm", G_FILE_TEST_IS_DIR))
                flatpak_bwrap_add_args (bwrap,
                                        "--bind", "/run/shm", "/run/shm",
                                        NULL);
              else
                flatpak_bwrap_add_args (bwrap,
                                        "--dir", "/run/shm",
                                        NULL);
            }
          else
            g_warning ("Unexpected /dev/shm symlink %s", link);
        }
    }
  else
    {
      flatpak_bwrap_add_args (bwrap,
                              "--dev", "/dev",
                              NULL);
      if (context->devices & FLATPAK_CONTEXT_DEVICE_DRI)
        {
          g_debug ("Allowing dri access");
          int i;
          char *dri_devices[] = {
            "/dev/dri",
            /* mali */
            "/dev/mali",
            "/dev/mali0",
            "/dev/umplock",
            /* nvidia */
            "/dev/nvidiactl",
            "/dev/nvidia-modeset",
            /* nvidia OpenCL/CUDA */
            "/dev/nvidia-uvm",
            "/dev/nvidia-uvm-tools",
          };

          for (i = 0; i < G_N_ELEMENTS (dri_devices); i++)
            {
              if (g_file_test (dri_devices[i], G_FILE_TEST_EXISTS))
                flatpak_bwrap_add_args (bwrap, "--dev-bind", dri_devices[i], dri_devices[i], NULL);
            }

          /* Each Nvidia card gets its own device.
             This is a fairly arbitrary limit but ASUS sells mining boards supporting 20 in theory. */
          char nvidia_dev[14]; /* /dev/nvidia plus up to 2 digits */
          for (i = 0; i < 20; i++)
            {
              g_snprintf (nvidia_dev, sizeof (nvidia_dev), "/dev/nvidia%d", i);
              if (g_file_test (nvidia_dev, G_FILE_TEST_EXISTS))
                flatpak_bwrap_add_args (bwrap, "--dev-bind", nvidia_dev, nvidia_dev, NULL);
            }
        }

      if (context->devices & FLATPAK_CONTEXT_DEVICE_KVM)
        {
          g_debug ("Allowing kvm access");
          if (g_file_test ("/dev/kvm", G_FILE_TEST_EXISTS))
            flatpak_bwrap_add_args (bwrap, "--dev-bind", "/dev/kvm", "/dev/kvm", NULL);
        }

      if (context->devices & FLATPAK_CONTEXT_DEVICE_SHM)
        {
          /* This is a symlink to /run/shm on debian, so bind to real target */
          g_autofree char *real_dev_shm = realpath ("/dev/shm", NULL);

          g_debug ("Allowing /dev/shm access (as %s)", real_dev_shm);
          if (real_dev_shm != NULL)
              flatpak_bwrap_add_args (bwrap, "--bind", real_dev_shm, "/dev/shm", NULL);
        }
    }

  flatpak_context_append_bwrap_filesystem (context, bwrap, app_id, app_id_dir, previous_app_id_dirs, &exports);

  if (context->sockets & FLATPAK_CONTEXT_SOCKET_WAYLAND)
    {
      g_debug ("Allowing wayland access");
      has_wayland = flatpak_run_add_wayland_args (bwrap);
    }

  if ((context->sockets & FLATPAK_CONTEXT_SOCKET_FALLBACK_X11) != 0)
    allow_x11 = !has_wayland;
  else
    allow_x11 = (context->sockets & FLATPAK_CONTEXT_SOCKET_X11) != 0;

  flatpak_run_add_x11_args (bwrap, allow_x11);

  if (context->sockets & FLATPAK_CONTEXT_SOCKET_SSH_AUTH)
    {
      flatpak_run_add_ssh_args (bwrap);
    }

  if (context->sockets & FLATPAK_CONTEXT_SOCKET_PULSEAUDIO)
    {
      g_debug ("Allowing pulseaudio access");
      flatpak_run_add_pulseaudio_args (bwrap);
    }

  if (context->sockets & FLATPAK_CONTEXT_SOCKET_PCSC)
    {
      flatpak_run_add_pcsc_args (bwrap);
    }

  if (context->sockets & FLATPAK_CONTEXT_SOCKET_CUPS)
    {
      flatpak_run_add_cups_args (bwrap);
    }

  flatpak_run_add_session_dbus_args (bwrap, proxy_arg_bwrap, context, flags, app_id);
  flatpak_run_add_system_dbus_args (bwrap, proxy_arg_bwrap, context, flags);
  flatpak_run_add_a11y_dbus_args (bwrap, proxy_arg_bwrap, context, flags);

  if (g_environ_getenv (bwrap->envp, "LD_LIBRARY_PATH") != NULL)
    {
      /* LD_LIBRARY_PATH is overridden for setuid helper, so pass it as cmdline arg */
      flatpak_bwrap_add_args (bwrap,
                              "--setenv", "LD_LIBRARY_PATH", g_environ_getenv (bwrap->envp, "LD_LIBRARY_PATH"),
                              NULL);
      flatpak_bwrap_unset_env (bwrap, "LD_LIBRARY_PATH");
    }

  if (g_environ_getenv (bwrap->envp, "TMPDIR") != NULL)
    {
      /* TMPDIR is overridden for setuid helper, so pass it as cmdline arg */
      flatpak_bwrap_add_args (bwrap,
                              "--setenv", "TMPDIR", g_environ_getenv (bwrap->envp, "TMPDIR"),
                              NULL);
      flatpak_bwrap_unset_env (bwrap, "TMPDIR");
    }

  /* Must run this before spawning the dbus proxy, to ensure it
     ends up in the app cgroup */
  if (!flatpak_run_in_transient_unit (app_id, &my_error))
    {
      /* We still run along even if we don't get a cgroup, as nothing
         really depends on it. Its just nice to have */
      g_debug ("Failed to run in transient scope: %s", my_error->message);
      g_clear_error (&my_error);
    }

  if (!flatpak_bwrap_is_empty (proxy_arg_bwrap) &&
      !start_dbus_proxy (bwrap, proxy_arg_bwrap, app_info_path, error))
    return FALSE;

  if (exports_out)
    *exports_out = g_steal_pointer (&exports);

  return TRUE;
}