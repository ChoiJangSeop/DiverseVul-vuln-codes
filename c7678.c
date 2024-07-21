g_keyfile_settings_backend_keyfile_write (GKeyfileSettingsBackend *kfsb)
{
  gchar *contents;
  gsize length;

  contents = g_key_file_to_data (kfsb->keyfile, &length, NULL);
  g_file_replace_contents (kfsb->file, contents, length, NULL, FALSE,
                           G_FILE_CREATE_REPLACE_DESTINATION,
                           NULL, NULL, NULL);

  compute_checksum (kfsb->digest, contents, length);
  g_free (contents);
}