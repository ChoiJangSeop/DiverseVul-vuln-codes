do_query_info_on_write (GVfsBackend *backend,
                        GVfsJobQueryInfoWrite *query_info_job,
                        GVfsBackendHandle handle,
                        GFileInfo *info,
                        GFileAttributeMatcher *matcher)
{
  GVfsJob *job = G_VFS_JOB (query_info_job);
  GFileOutputStream *stream = handle;
  GError *error = NULL;
  GFileInfo *real_info;

  real_info = g_file_output_stream_query_info (stream, query_info_job->attributes,
                                               job->cancellable, &error);
  if (error != NULL)
    goto out;

  fix_file_info (real_info);
  g_file_info_copy_into (real_info, info);
  g_object_unref (real_info);

 out:
  complete_job (job, error);
}