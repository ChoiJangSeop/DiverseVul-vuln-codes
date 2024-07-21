flatpak_run_app (FlatpakDecomposed *app_ref,
                 FlatpakDeploy     *app_deploy,
                 FlatpakContext    *extra_context,
                 const char        *custom_runtime,
                 const char        *custom_runtime_version,
                 const char        *custom_runtime_commit,
                 int                parent_pid,
                 FlatpakRunFlags    flags,
                 const char        *cwd,
                 const char        *custom_command,
                 char              *args[],
                 int                n_args,
                 int                instance_id_fd,
                 char             **instance_dir_out,
                 GCancellable      *cancellable,
                 GError           **error)
{
  g_autoptr(FlatpakDeploy) runtime_deploy = NULL;
  g_autoptr(GBytes) runtime_deploy_data = NULL;
  g_autoptr(GBytes) app_deploy_data = NULL;
  g_autoptr(GFile) app_files = NULL;
  g_autoptr(GFile) runtime_files = NULL;
  g_autoptr(GFile) bin_ldconfig = NULL;
  g_autoptr(GFile) app_id_dir = NULL;
  g_autoptr(GFile) real_app_id_dir = NULL;
  g_autofree char *default_runtime_pref = NULL;
  g_autoptr(FlatpakDecomposed) default_runtime = NULL;
  g_autofree char *default_command = NULL;
  g_autoptr(GKeyFile) metakey = NULL;
  g_autoptr(GKeyFile) runtime_metakey = NULL;
  g_autoptr(FlatpakBwrap) bwrap = NULL;
  const char *command = "/bin/sh";
  g_autoptr(GError) my_error = NULL;
  g_autoptr(FlatpakDecomposed) runtime_ref = NULL;
  int i;
  g_autoptr(GPtrArray) previous_app_id_dirs = NULL;
  g_autofree char *app_id = NULL;
  g_autofree char *app_arch = NULL;
  g_autofree char *app_info_path = NULL;
  g_autofree char *instance_id_host_dir = NULL;
  g_autoptr(FlatpakContext) app_context = NULL;
  g_autoptr(FlatpakContext) overrides = NULL;
  g_autoptr(FlatpakExports) exports = NULL;
  g_autofree char *commandline = NULL;
  g_autofree char *doc_mount_path = NULL;
  g_autofree char *app_extensions = NULL;
  g_autofree char *runtime_extensions = NULL;
  g_autofree char *checksum = NULL;
  int ld_so_fd = -1;
  g_autoptr(GFile) runtime_ld_so_conf = NULL;
  gboolean generate_ld_so_conf = TRUE;
  gboolean use_ld_so_cache = TRUE;
  gboolean sandboxed = (flags & FLATPAK_RUN_FLAG_SANDBOX) != 0;
  gboolean parent_expose_pids = (flags & FLATPAK_RUN_FLAG_PARENT_EXPOSE_PIDS) != 0;
  gboolean parent_share_pids = (flags & FLATPAK_RUN_FLAG_PARENT_SHARE_PIDS) != 0;
  struct stat s;

  if (!check_sudo (error))
    return FALSE;

  app_id = flatpak_decomposed_dup_id (app_ref);
  app_arch = flatpak_decomposed_dup_arch (app_ref);

  /* Check the user is allowed to run this flatpak. */
  if (!check_parental_controls (app_ref, app_deploy, cancellable, error))
    return FALSE;

  /* Construct the bwrap context. */
  bwrap = flatpak_bwrap_new (NULL);
  flatpak_bwrap_add_arg (bwrap, flatpak_get_bwrap ());

  if (app_deploy == NULL)
    {
      g_assert (flatpak_decomposed_is_runtime (app_ref));
      default_runtime_pref = flatpak_decomposed_dup_pref (app_ref);
    }
  else
    {
      const gchar *key;

      app_deploy_data = flatpak_deploy_get_deploy_data (app_deploy, FLATPAK_DEPLOY_VERSION_ANY, cancellable, error);
      if (app_deploy_data == NULL)
        return FALSE;

      if ((flags & FLATPAK_RUN_FLAG_DEVEL) != 0)
        key = FLATPAK_METADATA_KEY_SDK;
      else
        key = FLATPAK_METADATA_KEY_RUNTIME;

      metakey = flatpak_deploy_get_metadata (app_deploy);
      default_runtime_pref = g_key_file_get_string (metakey,
                                                    FLATPAK_METADATA_GROUP_APPLICATION,
                                                    key, &my_error);
      if (my_error)
        {
          g_propagate_error (error, g_steal_pointer (&my_error));
          return FALSE;
        }
    }

  default_runtime = flatpak_decomposed_new_from_pref (FLATPAK_KINDS_RUNTIME, default_runtime_pref, error);
  if (default_runtime == NULL)
    return FALSE;

  if (custom_runtime != NULL || custom_runtime_version != NULL)
    {
      g_auto(GStrv) custom_runtime_parts = NULL;
      const char *custom_runtime_id = NULL;
      const char *custom_runtime_arch = NULL;

      if (custom_runtime)
        {
          custom_runtime_parts = g_strsplit (custom_runtime, "/", 0);
          for (i = 0; i < 3 && custom_runtime_parts[i] != NULL; i++)
            {
              if (strlen (custom_runtime_parts[i]) > 0)
                {
                  if (i == 0)
                    custom_runtime_id = custom_runtime_parts[i];
                  if (i == 1)
                    custom_runtime_arch = custom_runtime_parts[i];

                  if (i == 2 && custom_runtime_version == NULL)
                    custom_runtime_version = custom_runtime_parts[i];
                }
            }
        }

      runtime_ref = flatpak_decomposed_new_from_decomposed (default_runtime,
                                                            FLATPAK_KINDS_RUNTIME,
                                                            custom_runtime_id,
                                                            custom_runtime_arch,
                                                            custom_runtime_version,
                                                            error);
      if (runtime_ref == NULL)
        return FALSE;
    }
  else
    runtime_ref = flatpak_decomposed_ref (default_runtime);

  runtime_deploy = flatpak_find_deploy_for_ref (flatpak_decomposed_get_ref (runtime_ref), custom_runtime_commit, NULL, cancellable, error);
  if (runtime_deploy == NULL)
    return FALSE;

  runtime_deploy_data = flatpak_deploy_get_deploy_data (runtime_deploy, FLATPAK_DEPLOY_VERSION_ANY, cancellable, error);
  if (runtime_deploy_data == NULL)
    return FALSE;

  runtime_metakey = flatpak_deploy_get_metadata (runtime_deploy);

  app_context = flatpak_app_compute_permissions (metakey, runtime_metakey, error);
  if (app_context == NULL)
    return FALSE;

  if (app_deploy != NULL)
    {
      overrides = flatpak_deploy_get_overrides (app_deploy);
      flatpak_context_merge (app_context, overrides);
    }

  if (sandboxed)
    flatpak_context_make_sandboxed (app_context);

  if (extra_context)
    flatpak_context_merge (app_context, extra_context);

  runtime_files = flatpak_deploy_get_files (runtime_deploy);
  bin_ldconfig = g_file_resolve_relative_path (runtime_files, "bin/ldconfig");
  if (!g_file_query_exists (bin_ldconfig, NULL))
    use_ld_so_cache = FALSE;

  if (app_deploy != NULL)
    {
      g_autofree const char **previous_ids = NULL;
      gsize len = 0;
      gboolean do_migrate;

      real_app_id_dir = flatpak_get_data_dir (app_id);
      app_files = flatpak_deploy_get_files (app_deploy);

      previous_app_id_dirs = g_ptr_array_new_with_free_func (g_object_unref);
      previous_ids = flatpak_deploy_data_get_previous_ids (app_deploy_data, &len);

      do_migrate = !g_file_query_exists (real_app_id_dir, cancellable);

      /* When migrating, find most recent old existing source and rename that to
       * the new name.
       *
       * We ignore other names than that. For more recent names that don't exist
       * we never ran them so nothing will even reference them. For older names
       * either they were not used, or they were used but then the more recent
       * name was used and a symlink to it was created.
       *
       * This means we may end up with a chain of symlinks: oldest -> old -> current.
       * This is unfortunate but not really a problem, but for robustness reasons we
       * don't want to mess with user files unnecessary. For example, the app dir could
       * actually be a symlink for other reasons. Imagine for instance that you want to put the
       * steam games somewhere else so you leave the app dir as a symlink to /mnt/steam.
       */
      for (i = len - 1; i >= 0; i--)
        {
          g_autoptr(GFile) previous_app_id_dir = NULL;
          g_autoptr(GFileInfo) previous_app_id_dir_info = NULL;
          g_autoptr(GError) local_error = NULL;

          previous_app_id_dir = flatpak_get_data_dir (previous_ids[i]);
          previous_app_id_dir_info = g_file_query_info (previous_app_id_dir,
                                                        G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK ","
                                                        G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
                                                        G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                                        cancellable,
                                                        &local_error);
          /* Warn about the migration failures, but don't make them fatal, then you can never run the app */
          if (previous_app_id_dir_info == NULL)
            {
              if  (!g_error_matches (local_error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND) && do_migrate)
                {
                  g_warning (_("Failed to migrate from %s: %s"), flatpak_file_get_path_cached (previous_app_id_dir),
                             local_error->message);
                  do_migrate = FALSE; /* Don't migrate older things, they are likely symlinks to the thing that we failed on */
                }

              g_clear_error (&local_error);
              continue;
            }

          if (do_migrate)
            {
              do_migrate = FALSE; /* Don't migrate older things, they are likely symlinks to this dir */

              if (!flatpak_file_rename (previous_app_id_dir, real_app_id_dir, cancellable, &local_error))
                {
                  g_warning (_("Failed to migrate old app data directory %s to new name %s: %s"),
                             flatpak_file_get_path_cached (previous_app_id_dir), app_id,
                             local_error->message);
                }
              else
                {
                  /* Leave a symlink in place of the old data dir */
                  if (!g_file_make_symbolic_link (previous_app_id_dir, app_id, cancellable, &local_error))
                    {
                      g_warning (_("Failed to create symlink while migrating %s: %s"),
                                 flatpak_file_get_path_cached (previous_app_id_dir),
                                 local_error->message);
                    }
                }
            }

          /* Give app access to this old dir */
          g_ptr_array_add (previous_app_id_dirs, g_steal_pointer (&previous_app_id_dir));
        }

      if (!flatpak_ensure_data_dir (real_app_id_dir, cancellable, error))
        return FALSE;

      if (!sandboxed)
        app_id_dir = g_object_ref (real_app_id_dir);
    }

  flatpak_run_apply_env_default (bwrap, use_ld_so_cache);
  flatpak_run_apply_env_vars (bwrap, app_context);
  flatpak_run_apply_env_prompt (bwrap, app_id);

  if (real_app_id_dir)
    {
      g_autoptr(GFile) sandbox_dir = g_file_get_child (real_app_id_dir, "sandbox");
      flatpak_bwrap_set_env (bwrap, "FLATPAK_SANDBOX_DIR", flatpak_file_get_path_cached (sandbox_dir), TRUE);
    }

  flatpak_bwrap_add_args (bwrap,
                          "--ro-bind", flatpak_file_get_path_cached (runtime_files), "/usr",
                          "--lock-file", "/usr/.ref",
                          NULL);

  if (app_files != NULL)
    flatpak_bwrap_add_args (bwrap,
                            "--ro-bind", flatpak_file_get_path_cached (app_files), "/app",
                            "--lock-file", "/app/.ref",
                            NULL);
  else
    flatpak_bwrap_add_args (bwrap,
                            "--dir", "/app",
                            NULL);

  if (metakey != NULL &&
      !flatpak_run_add_extension_args (bwrap, metakey, app_ref, use_ld_so_cache, &app_extensions, cancellable, error))
    return FALSE;

  if (!flatpak_run_add_extension_args (bwrap, runtime_metakey, runtime_ref, use_ld_so_cache, &runtime_extensions, cancellable, error))
    return FALSE;

  runtime_ld_so_conf = g_file_resolve_relative_path (runtime_files, "etc/ld.so.conf");
  if (lstat (flatpak_file_get_path_cached (runtime_ld_so_conf), &s) == 0)
    generate_ld_so_conf = S_ISREG (s.st_mode) && s.st_size == 0;

  /* At this point we have the minimal argv set up, with just the app, runtime and extensions.
     We can reuse this to generate the ld.so.cache (if needed) */
  if (use_ld_so_cache)
    {
      checksum = calculate_ld_cache_checksum (app_deploy_data, runtime_deploy_data,
                                              app_extensions, runtime_extensions);
      ld_so_fd = regenerate_ld_cache (bwrap->argv,
                                      bwrap->fds,
                                      app_id_dir,
                                      checksum,
                                      runtime_files,
                                      generate_ld_so_conf,
                                      cancellable, error);
      if (ld_so_fd == -1)
        return FALSE;
      flatpak_bwrap_add_fd (bwrap, ld_so_fd);
    }

  flags |= flatpak_context_get_run_flags (app_context);

  if (!flatpak_run_setup_base_argv (bwrap, runtime_files, app_id_dir, app_arch, flags, error))
    return FALSE;

  if (generate_ld_so_conf)
    {
      if (!add_ld_so_conf (bwrap, error))
        return FALSE;
    }

  if (ld_so_fd != -1)
    {
      /* Don't add to fd_array, its already there */
      flatpak_bwrap_add_arg (bwrap, "--ro-bind-data");
      flatpak_bwrap_add_arg_printf (bwrap, "%d", ld_so_fd);
      flatpak_bwrap_add_arg (bwrap, "/etc/ld.so.cache");
    }

  if (!flatpak_run_add_app_info_args (bwrap,
                                      app_files, app_deploy_data, app_extensions,
                                      runtime_files, runtime_deploy_data, runtime_extensions,
                                      app_id, flatpak_decomposed_get_branch (app_ref),
                                      runtime_ref, app_id_dir, app_context, extra_context,
                                      sandboxed, FALSE, flags & FLATPAK_RUN_FLAG_DEVEL,
                                      &app_info_path, instance_id_fd, &instance_id_host_dir,
                                      error))
    return FALSE;

  if (!flatpak_run_add_dconf_args (bwrap, app_id, metakey, error))
    return FALSE;

  if (!sandboxed && !(flags & FLATPAK_RUN_FLAG_NO_DOCUMENTS_PORTAL))
    add_document_portal_args (bwrap, app_id, &doc_mount_path);

  if (!flatpak_run_add_environment_args (bwrap, app_info_path, flags,
                                         app_id, app_context, app_id_dir, previous_app_id_dirs,
                                         &exports, cancellable, error))
    return FALSE;

  if ((app_context->shares & FLATPAK_CONTEXT_SHARED_NETWORK) != 0)
    flatpak_run_add_resolved_args (bwrap);

  flatpak_run_add_journal_args (bwrap);
  add_font_path_args (bwrap);
  add_icon_path_args (bwrap);

  flatpak_bwrap_add_args (bwrap,
                          /* Not in base, because we don't want this for flatpak build */
                          "--symlink", "/app/lib/debug/source", "/run/build",
                          "--symlink", "/usr/lib/debug/source", "/run/build-runtime",
                          NULL);

  if (cwd)
    flatpak_bwrap_add_args (bwrap, "--chdir", cwd, NULL);

  if (parent_expose_pids || parent_share_pids)
    {
      g_autofree char *userns_path = NULL;
      g_autofree char *pidns_path = NULL;
      g_autofree char *userns2_path = NULL;
      int userns_fd, userns2_fd, pidns_fd;

      if (parent_pid == 0)
        return flatpak_fail (error, "No parent pid specified");

      userns_path = g_strdup_printf ("/proc/%d/root/run/.userns", parent_pid);

      userns_fd = open_namespace_fd_if_needed (userns_path, "/proc/self/ns/user");
      if (userns_fd != -1)
        {
          flatpak_bwrap_add_args_data_fd (bwrap, "--userns", userns_fd, NULL);

          userns2_path = g_strdup_printf ("/proc/%d/ns/user", parent_pid);
          userns2_fd = open_namespace_fd_if_needed (userns2_path, userns_path);
          if (userns2_fd != -1)
            flatpak_bwrap_add_args_data_fd (bwrap, "--userns2", userns2_fd, NULL);
        }

      pidns_path = g_strdup_printf ("/proc/%d/ns/pid", parent_pid);
      pidns_fd = open (pidns_path, O_RDONLY|O_CLOEXEC);
      if (pidns_fd != -1)
        flatpak_bwrap_add_args_data_fd (bwrap, "--pidns", pidns_fd, NULL);
    }

  if (custom_command)
    {
      command = custom_command;
    }
  else if (metakey)
    {
      default_command = g_key_file_get_string (metakey,
                                               FLATPAK_METADATA_GROUP_APPLICATION,
                                               FLATPAK_METADATA_KEY_COMMAND,
                                               &my_error);
      if (my_error)
        {
          g_propagate_error (error, g_steal_pointer (&my_error));
          return FALSE;
        }
      command = default_command;
    }

  if (!flatpak_bwrap_bundle_args (bwrap, 1, -1, FALSE, error))
    return FALSE;

  flatpak_bwrap_add_arg (bwrap, command);

  if (!add_rest_args (bwrap, app_id,
                      exports, (flags & FLATPAK_RUN_FLAG_FILE_FORWARDING) != 0,
                      doc_mount_path,
                      args, n_args, error))
    return FALSE;

  flatpak_bwrap_finish (bwrap);

  commandline = flatpak_quote_argv ((const char **) bwrap->argv->pdata, -1);
  g_debug ("Running '%s'", commandline);

  if ((flags & FLATPAK_RUN_FLAG_BACKGROUND) != 0)
    {
      GPid child_pid;
      char pid_str[64];
      g_autofree char *pid_path = NULL;
      GSpawnFlags spawn_flags;

      spawn_flags = G_SPAWN_SEARCH_PATH;
      if (flags & FLATPAK_RUN_FLAG_DO_NOT_REAP)
        spawn_flags |= G_SPAWN_DO_NOT_REAP_CHILD;

      /* We use LEAVE_DESCRIPTORS_OPEN to work around dead-lock, see flatpak_close_fds_workaround */
      spawn_flags |= G_SPAWN_LEAVE_DESCRIPTORS_OPEN;

      if (!g_spawn_async (NULL,
                          (char **) bwrap->argv->pdata,
                          bwrap->envp,
                          spawn_flags,
                          flatpak_bwrap_child_setup_cb, bwrap->fds,
                          &child_pid,
                          error))
        return FALSE;

      g_snprintf (pid_str, sizeof (pid_str), "%d", child_pid);
      pid_path = g_build_filename (instance_id_host_dir, "pid", NULL);
      g_file_set_contents (pid_path, pid_str, -1, NULL);
    }
  else
    {
      char pid_str[64];
      g_autofree char *pid_path = NULL;

      g_snprintf (pid_str, sizeof (pid_str), "%d", getpid ());
      pid_path = g_build_filename (instance_id_host_dir, "pid", NULL);
      g_file_set_contents (pid_path, pid_str, -1, NULL);

      /* Ensure we unset O_CLOEXEC for marked fds and rewind fds as needed.
       * Note that this does not close fds that are not already marked O_CLOEXEC, because
       * we do want to allow inheriting fds into flatpak run. */
      flatpak_bwrap_child_setup (bwrap->fds, FALSE);

      if (execvpe (flatpak_get_bwrap (), (char **) bwrap->argv->pdata, bwrap->envp) == -1)
        {
          g_set_error_literal (error, G_IO_ERROR, g_io_error_from_errno (errno),
                               _("Unable to start app"));
          return FALSE;
        }
      /* Not actually reached... */
    }

  if (instance_dir_out)
    *instance_dir_out = g_steal_pointer (&instance_id_host_dir);

  return TRUE;
}