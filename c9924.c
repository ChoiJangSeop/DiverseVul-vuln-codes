apply_extra_data (FlatpakDir   *self,
                  GFile        *checkoutdir,
                  GCancellable *cancellable,
                  GError      **error)
{
  g_autoptr(GFile) metadata = NULL;
  g_autofree char *metadata_contents = NULL;
  gsize metadata_size;
  g_autoptr(GKeyFile) metakey = NULL;
  g_autofree char *id = NULL;
  g_autofree char *runtime_pref = NULL;
  g_autoptr(FlatpakDecomposed) runtime_ref = NULL;
  g_autoptr(FlatpakDeploy) runtime_deploy = NULL;
  g_autoptr(FlatpakBwrap) bwrap = NULL;
  g_autoptr(GFile) app_files = NULL;
  g_autoptr(GFile) apply_extra_file = NULL;
  g_autoptr(GFile) app_export_file = NULL;
  g_autoptr(GFile) extra_export_file = NULL;
  g_autoptr(GFile) extra_files = NULL;
  g_autoptr(GFile) runtime_files = NULL;
  g_autoptr(FlatpakContext) app_context = NULL;
  g_auto(GStrv) minimal_envp = NULL;
  g_autofree char *runtime_arch = NULL;
  int exit_status;
  const char *group = FLATPAK_METADATA_GROUP_APPLICATION;
  g_autoptr(GError) local_error = NULL;

  apply_extra_file = g_file_resolve_relative_path (checkoutdir, "files/bin/apply_extra");
  if (!g_file_query_exists (apply_extra_file, cancellable))
    return TRUE;

  metadata = g_file_get_child (checkoutdir, "metadata");

  if (!g_file_load_contents (metadata, cancellable, &metadata_contents, &metadata_size, NULL, error))
    return FALSE;

  metakey = g_key_file_new ();
  if (!g_key_file_load_from_data (metakey, metadata_contents, metadata_size, 0, error))
    return FALSE;

  id = g_key_file_get_string (metakey, group, FLATPAK_METADATA_KEY_NAME,
                              &local_error);
  if (id == NULL)
    {
      group = FLATPAK_METADATA_GROUP_RUNTIME;
      id = g_key_file_get_string (metakey, group, FLATPAK_METADATA_KEY_NAME,
                                  NULL);
      if (id == NULL)
        {
          g_propagate_error (error, g_steal_pointer (&local_error));
          return FALSE;
        }
      g_clear_error (&local_error);
    }

  runtime_pref = g_key_file_get_string (metakey, group,
                                        FLATPAK_METADATA_KEY_RUNTIME, error);
  if (runtime_pref == NULL)
    runtime_pref = g_key_file_get_string (metakey, FLATPAK_METADATA_GROUP_EXTENSION_OF,
                                          FLATPAK_METADATA_KEY_RUNTIME, NULL);
  if (runtime_pref == NULL)
    return FALSE;

  runtime_ref = flatpak_decomposed_new_from_pref (FLATPAK_KINDS_RUNTIME, runtime_pref, error);
  if (runtime_ref == NULL)
    return FALSE;
  runtime_arch = flatpak_decomposed_dup_arch (runtime_ref);

  if (!g_key_file_get_boolean (metakey, FLATPAK_METADATA_GROUP_EXTRA_DATA,
                               FLATPAK_METADATA_KEY_NO_RUNTIME, NULL))
    {
      /* We pass in self here so that we ensure that we find the runtime in case it only
         exists in this installation (which might be custom) */
      runtime_deploy = flatpak_find_deploy_for_ref (flatpak_decomposed_get_ref (runtime_ref), NULL, self, cancellable, error);
      if (runtime_deploy == NULL)
        return FALSE;
      runtime_files = flatpak_deploy_get_files (runtime_deploy);
    }

  app_files = g_file_get_child (checkoutdir, "files");
  app_export_file = g_file_get_child (checkoutdir, "export");
  extra_files = g_file_get_child (app_files, "extra");
  extra_export_file = g_file_get_child (extra_files, "export");

  minimal_envp = flatpak_run_get_minimal_env (FALSE, FALSE);
  bwrap = flatpak_bwrap_new (minimal_envp);
  flatpak_bwrap_add_args (bwrap, flatpak_get_bwrap (), NULL);

  if (runtime_files)
    flatpak_bwrap_add_args (bwrap,
                            "--ro-bind", flatpak_file_get_path_cached (runtime_files), "/usr",
                            "--lock-file", "/usr/.ref",
                            NULL);

  flatpak_bwrap_add_args (bwrap,
                          "--ro-bind", flatpak_file_get_path_cached (app_files), "/app",
                          "--bind", flatpak_file_get_path_cached (extra_files), "/app/extra",
                          "--chdir", "/app/extra",
                          /* We run as root in the system-helper case, so drop all caps */
                          "--cap-drop", "ALL",
                          NULL);

  if (!flatpak_run_setup_base_argv (bwrap, runtime_files, NULL, runtime_arch,
                                    /* Might need multiarch in apply_extra (see e.g. #3742). Should be pretty safe in this limited context */
                                    FLATPAK_RUN_FLAG_MULTIARCH |
                                    FLATPAK_RUN_FLAG_NO_SESSION_HELPER | FLATPAK_RUN_FLAG_NO_PROC,
                                    error))
    return FALSE;

  app_context = flatpak_context_new ();

  if (!flatpak_run_add_environment_args (bwrap, NULL,
                                         FLATPAK_RUN_FLAG_NO_SESSION_BUS_PROXY |
                                         FLATPAK_RUN_FLAG_NO_SYSTEM_BUS_PROXY |
                                         FLATPAK_RUN_FLAG_NO_A11Y_BUS_PROXY,
                                         id,
                                         app_context, NULL, NULL, NULL, cancellable, error))
    return FALSE;

  flatpak_bwrap_add_arg (bwrap, "/app/bin/apply_extra");

  flatpak_bwrap_finish (bwrap);

  g_debug ("Running /app/bin/apply_extra ");

  /* We run the sandbox without caps, but it can still create files owned by itself with
   * arbitrary permissions, including setuid myself. This is extra risky in the case where
   * this runs as root in the system helper case. We canonicalize the permissions at the
   * end, but to avoid non-canonical permissions leaking out before then we make the
   * toplevel dir only accessible to the user */
  if (chmod (flatpak_file_get_path_cached (extra_files), 0700) != 0)
    {
      glnx_set_error_from_errno (error);
      return FALSE;
    }

  if (!g_spawn_sync (NULL,
                     (char **) bwrap->argv->pdata,
                     bwrap->envp,
                     G_SPAWN_SEARCH_PATH,
                     child_setup, bwrap->fds,
                     NULL, NULL,
                     &exit_status,
                     error))
    return FALSE;

  if (!flatpak_canonicalize_permissions (AT_FDCWD, flatpak_file_get_path_cached (extra_files),
                                         getuid () == 0 ? 0 : -1,
                                         getuid () == 0 ? 0 : -1,
                                         error))
    return FALSE;

  if (exit_status != 0)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   _("apply_extra script failed, exit status %d"), exit_status);
      return FALSE;
    }

  if (g_file_query_exists (extra_export_file, cancellable))
    {
      if (!flatpak_mkdir_p (app_export_file, cancellable, error))
        return FALSE;
      if (!flatpak_cp_a (extra_export_file,
                         app_export_file,
                         FLATPAK_CP_FLAGS_MERGE,
                         cancellable, error))
        return FALSE;
    }

  return TRUE;
}