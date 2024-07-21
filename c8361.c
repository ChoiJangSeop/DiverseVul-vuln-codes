create_face_from_contents (FontLoadJob *job,
                           gchar **contents,
                           GError **error)
{
  FT_Error ft_error;
  FT_Face retval;

  ft_error = FT_New_Memory_Face (job->library,
                                 (const FT_Byte *) job->face_contents,
                                 (FT_Long) job->face_length,
                                 job->face_index,
                                 &retval);

  if (ft_error != 0) {
    g_autofree gchar *uri = g_file_get_uri (job->file);
    g_set_error (error, G_IO_ERROR, 0,
                 "Unable to read the font face file '%s'", uri);
    return NULL;
  }

  *contents = g_steal_pointer (&job->face_contents);
  return retval;
}