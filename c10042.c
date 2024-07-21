autoar_extractor_do_sanitize_pathname (AutoarExtractor *self,
                                       const char      *pathname_bytes)
{
  GFile *extracted_filename;
  gboolean valid_filename;
  g_autofree char *sanitized_pathname;
  g_autofree char *utf8_pathname;

  utf8_pathname = autoar_common_get_utf8_pathname (pathname_bytes);
  extracted_filename = g_file_get_child (self->destination_dir,
                                         utf8_pathname ?  utf8_pathname : pathname_bytes);

  valid_filename =
    g_file_equal (extracted_filename, self->destination_dir) ||
    g_file_has_prefix (extracted_filename, self->destination_dir);

  if (!valid_filename) {
    g_autofree char *basename;

    basename = g_file_get_basename (extracted_filename);

    g_object_unref (extracted_filename);

    extracted_filename = g_file_get_child (self->destination_dir,
                                           basename);
  }

  if (self->prefix != NULL && self->new_prefix != NULL) {
    g_autofree char *relative_path;
    /* Replace the old prefix with the new one */
    relative_path = g_file_get_relative_path (self->prefix,
                                              extracted_filename);

    relative_path = relative_path != NULL ? relative_path : g_strdup ("");

    g_object_unref (extracted_filename);

    extracted_filename = g_file_get_child (self->new_prefix,
                                           relative_path);
  }

  sanitized_pathname = g_file_get_path (extracted_filename);

  g_debug ("autoar_extractor_do_sanitize_pathname: %s", sanitized_pathname);

  return extracted_filename;
}