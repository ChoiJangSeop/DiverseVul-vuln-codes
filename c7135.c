flatpak_run_setup_base_argv (FlatpakBwrap   *bwrap,
                             GFile          *runtime_files,
                             GFile          *app_id_dir,
                             const char     *arch,
                             FlatpakRunFlags flags,
                             GError        **error)
{
  g_autofree char *run_dir = NULL;
  g_autofree char *passwd_contents = NULL;
  g_autofree char *group_contents = NULL;
  const char *pkcs11_conf_contents = NULL;
  struct group *g;
  gulong pers;
  gid_t gid = getgid ();

  g_autoptr(GFile) etc = NULL;

  g = getgrgid (gid);
  if (g == NULL)
    return flatpak_fail_error (error, FLATPAK_ERROR_SETUP_FAILED, _("Invalid group: %d"), gid);

  run_dir = g_strdup_printf ("/run/user/%d", getuid ());

  passwd_contents = g_strdup_printf ("%s:x:%d:%d:%s:%s:%s\n"
                                     "nfsnobody:x:65534:65534:Unmapped user:/:/sbin/nologin\n",
                                     g_get_user_name (),
                                     getuid (), gid,
                                     g_get_real_name (),
                                     g_get_home_dir (),
                                     DEFAULT_SHELL);

  group_contents = g_strdup_printf ("%s:x:%d:%s\n"
                                    "nfsnobody:x:65534:\n",
                                    g->gr_name,
                                    gid, g_get_user_name ());

  pkcs11_conf_contents =
    "# Disable user pkcs11 config, because the host modules don't work in the runtime\n"
    "user-config: none\n";

  flatpak_bwrap_add_args (bwrap,
                          "--unshare-pid",
                          "--proc", "/proc",
                          "--dir", "/tmp",
                          "--dir", "/var/tmp",
                          "--dir", "/run/host",
                          "--dir", run_dir,
                          "--setenv", "XDG_RUNTIME_DIR", run_dir,
                          "--symlink", "../run", "/var/run",
                          "--ro-bind", "/sys/block", "/sys/block",
                          "--ro-bind", "/sys/bus", "/sys/bus",
                          "--ro-bind", "/sys/class", "/sys/class",
                          "--ro-bind", "/sys/dev", "/sys/dev",
                          "--ro-bind", "/sys/devices", "/sys/devices",
                          /* glib uses this like /etc/timezone */
                          "--symlink", "/etc/timezone", "/var/db/zoneinfo",
                          NULL);

  if (flags & FLATPAK_RUN_FLAG_DIE_WITH_PARENT)
    flatpak_bwrap_add_args (bwrap,
                            "--die-with-parent",
                            NULL);

  if (flags & FLATPAK_RUN_FLAG_WRITABLE_ETC)
    flatpak_bwrap_add_args (bwrap,
                            "--dir", "/usr/etc",
                            "--symlink", "usr/etc", "/etc",
                            NULL);

  if (!flatpak_bwrap_add_args_data (bwrap, "passwd", passwd_contents, -1, "/etc/passwd", error))
    return FALSE;

  if (!flatpak_bwrap_add_args_data (bwrap, "group", group_contents, -1, "/etc/group", error))
    return FALSE;

  if (!flatpak_bwrap_add_args_data (bwrap, "pkcs11.conf", pkcs11_conf_contents, -1, "/etc/pkcs11/pkcs11.conf", error))
    return FALSE;

  if (g_file_test ("/etc/machine-id", G_FILE_TEST_EXISTS))
    flatpak_bwrap_add_args (bwrap, "--ro-bind", "/etc/machine-id", "/etc/machine-id", NULL);
  else if (g_file_test ("/var/lib/dbus/machine-id", G_FILE_TEST_EXISTS))
    flatpak_bwrap_add_args (bwrap, "--ro-bind", "/var/lib/dbus/machine-id", "/etc/machine-id", NULL);

  if (runtime_files)
    etc = g_file_get_child (runtime_files, "etc");
  if (etc != NULL &&
      (flags & FLATPAK_RUN_FLAG_WRITABLE_ETC) == 0 &&
      g_file_query_exists (etc, NULL))
    {
      g_auto(GLnxDirFdIterator) dfd_iter = { 0, };
      struct dirent *dent;
      char path_buffer[PATH_MAX + 1];
      ssize_t symlink_size;
      gboolean inited;

      inited = glnx_dirfd_iterator_init_at (AT_FDCWD, flatpak_file_get_path_cached (etc), FALSE, &dfd_iter, NULL);

      while (inited)
        {
          g_autofree char *src = NULL;
          g_autofree char *dest = NULL;

          if (!glnx_dirfd_iterator_next_dent_ensure_dtype (&dfd_iter, &dent, NULL, NULL) || dent == NULL)
            break;

          if (strcmp (dent->d_name, "passwd") == 0 ||
              strcmp (dent->d_name, "group") == 0 ||
              strcmp (dent->d_name, "machine-id") == 0 ||
              strcmp (dent->d_name, "resolv.conf") == 0 ||
              strcmp (dent->d_name, "host.conf") == 0 ||
              strcmp (dent->d_name, "hosts") == 0 ||
              strcmp (dent->d_name, "localtime") == 0 ||
              strcmp (dent->d_name, "timezone") == 0 ||
              strcmp (dent->d_name, "pkcs11") == 0)
            continue;

          src = g_build_filename (flatpak_file_get_path_cached (etc), dent->d_name, NULL);
          dest = g_build_filename ("/etc", dent->d_name, NULL);
          if (dent->d_type == DT_LNK)
            {
              symlink_size = readlinkat (dfd_iter.fd, dent->d_name, path_buffer, sizeof (path_buffer) - 1);
              if (symlink_size < 0)
                {
                  glnx_set_error_from_errno (error);
                  return FALSE;
                }
              path_buffer[symlink_size] = 0;
              flatpak_bwrap_add_args (bwrap, "--symlink", path_buffer, dest, NULL);
            }
          else
            {
              flatpak_bwrap_add_args (bwrap, "--ro-bind", src, dest, NULL);
            }
        }
    }

  if (app_id_dir != NULL)
    {
      g_autoptr(GFile) app_cache_dir = g_file_get_child (app_id_dir, "cache");
      g_autoptr(GFile) app_tmp_dir = g_file_get_child (app_cache_dir, "tmp");
      g_autoptr(GFile) app_data_dir = g_file_get_child (app_id_dir, "data");
      g_autoptr(GFile) app_config_dir = g_file_get_child (app_id_dir, "config");

      flatpak_bwrap_add_args (bwrap,
                              /* These are nice to have as a fixed path */
                              "--bind", flatpak_file_get_path_cached (app_cache_dir), "/var/cache",
                              "--bind", flatpak_file_get_path_cached (app_data_dir), "/var/data",
                              "--bind", flatpak_file_get_path_cached (app_config_dir), "/var/config",
                              "--bind", flatpak_file_get_path_cached (app_tmp_dir), "/var/tmp",
                              NULL);
    }

  flatpak_run_setup_usr_links (bwrap, runtime_files);

  pers = PER_LINUX;

  if ((flags & FLATPAK_RUN_FLAG_SET_PERSONALITY) &&
      flatpak_is_linux32_arch (arch))
    {
      g_debug ("Setting personality linux32");
      pers = PER_LINUX32;
    }

  /* Always set the personallity, and clear all weird flags */
  personality (pers);

#ifdef ENABLE_SECCOMP
  if (!setup_seccomp (bwrap, arch, pers, flags, error))
    return FALSE;
#endif

  if ((flags & FLATPAK_RUN_FLAG_WRITABLE_ETC) == 0)
    add_monitor_path_args ((flags & FLATPAK_RUN_FLAG_NO_SESSION_HELPER) == 0, bwrap);

  return TRUE;
}