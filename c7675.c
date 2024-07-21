do_query_info (GVfsBackend *backend,
               GVfsJobQueryInfo *query_info_job,
               const char *filename,
               GFileQueryInfoFlags flags,
               GFileInfo *info,
               GFileAttributeMatcher *matcher)
{
  GVfsBackendAdmin *self = G_VFS_BACKEND_ADMIN (backend);
  GVfsJob *job = G_VFS_JOB (query_info_job);
  GError *error = NULL;
  GFile *file;
  GFileInfo *real_info;

  if (!check_permission (self, job))
    return;

  file = g_file_new_for_path (filename);
  real_info = g_file_query_info (file, query_info_job->attributes,
                                 flags, job->cancellable, &error);
  g_object_unref (file);

  if (error != NULL)
    goto out;

  fix_file_info (real_info);
  g_file_info_copy_into (real_info, info);
  g_object_unref (real_info);

 out:
  complete_job (job, error);
}