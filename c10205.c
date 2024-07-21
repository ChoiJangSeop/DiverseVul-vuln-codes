autoar_extractor_check_file_conflict (GFile  *file,
                                      mode_t  extracted_filetype)
{
  GFileType file_type;

  file_type = g_file_query_file_type (file,
                                      G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                      NULL);
  /* If there is no file with the given name, there will be no conflict */
  if (file_type == G_FILE_TYPE_UNKNOWN) {
    return FALSE;
  }

  /* It is not problem if the directory already exists */
  if (file_type == G_FILE_TYPE_DIRECTORY &&
      extracted_filetype == AE_IFDIR) {
    return FALSE;
  }

  return TRUE;
}