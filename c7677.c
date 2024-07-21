g_keyfile_settings_backend_constructed (GObject *object)
{
  GKeyfileSettingsBackend *kfsb = G_KEYFILE_SETTINGS_BACKEND (object);

  if (kfsb->file == NULL)
    {
      char *filename = g_build_filename (g_get_user_config_dir (),
                                         "glib-2.0", "settings", "keyfile",
                                         NULL);
      kfsb->file = g_file_new_for_path (filename);
      g_free (filename);
    }

  if (kfsb->prefix == NULL)
    {
      kfsb->prefix = g_strdup ("/");
      kfsb->prefix_len = 1;
    }
  
  kfsb->keyfile = g_key_file_new ();
  kfsb->permission = g_simple_permission_new (TRUE);

  kfsb->dir = g_file_get_parent (kfsb->file);
  g_file_make_directory_with_parents (kfsb->dir, NULL, NULL);

  kfsb->file_monitor = g_file_monitor (kfsb->file, G_FILE_MONITOR_NONE, NULL, NULL);
  kfsb->dir_monitor = g_file_monitor (kfsb->dir, G_FILE_MONITOR_NONE, NULL, NULL);

  compute_checksum (kfsb->digest, NULL, 0);

  g_signal_connect (kfsb->file_monitor, "changed",
                    G_CALLBACK (file_changed), kfsb);
  g_signal_connect (kfsb->dir_monitor, "changed",
                    G_CALLBACK (dir_changed), kfsb);

  g_keyfile_settings_backend_keyfile_writable (kfsb);
  g_keyfile_settings_backend_keyfile_reload (kfsb);

  load_system_settings (kfsb);
}