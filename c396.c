mark_trusted_job (GIOSchedulerJob *io_job,
		  GCancellable *cancellable,
		  gpointer user_data)
{
	MarkTrustedJob *job = user_data;
	CommonJob *common;
	char *contents, *new_contents;
	gsize length, new_length;
	GError *error;
	guint32 current;
	int response;
	GFileInfo *info;
	
	common = (CommonJob *)job;
	common->io_job = io_job;
	
	nautilus_progress_info_start (job->common.progress);

 retry:
	error = NULL;
	if (!g_file_load_contents (job->file,
				  cancellable,
				  &contents, &length,
				  NULL, &error)) {
		response = run_error (common,
				      g_strdup (_("Unable to mark launcher trusted (executable)")),
				      error->message,
				      NULL,
				      FALSE,
				      GTK_STOCK_CANCEL, RETRY,
				      NULL);

		if (response == 0 || response == GTK_RESPONSE_DELETE_EVENT) {
			abort_job (common);
		} else if (response == 1) {
			goto retry;
		} else {
			g_assert_not_reached ();
		}

		goto out;
	}

	if (!g_str_has_prefix (contents, "#!")) {
		new_length = length + strlen (TRUSTED_SHEBANG);
		new_contents = g_malloc (new_length);
		
		strcpy (new_contents, TRUSTED_SHEBANG);
		memcpy (new_contents + strlen (TRUSTED_SHEBANG),
			contents, length);
		
		if (!g_file_replace_contents (job->file,
					      new_contents,
					      new_length,
					      NULL,
					      FALSE, 0,
					      NULL, cancellable, &error)) {
			g_free (contents);
			g_free (new_contents);
			
			response = run_error (common,
					      g_strdup (_("Unable to mark launcher trusted (executable)")),
					      error->message,
					      NULL,
					      FALSE,
					      GTK_STOCK_CANCEL, RETRY,
					      NULL);
			
			if (response == 0 || response == GTK_RESPONSE_DELETE_EVENT) {
				abort_job (common);
			} else if (response == 1) {
				goto retry;
			} else {
				g_assert_not_reached ();
			}
			
			goto out;
		}
		g_free (new_contents);
		
	}
	g_free (contents);
	
	info = g_file_query_info (job->file,
				  G_FILE_ATTRIBUTE_STANDARD_TYPE","
				  G_FILE_ATTRIBUTE_UNIX_MODE,
				  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
				  common->cancellable,
				  &error);

	if (info == NULL) {
		response = run_error (common,
				      g_strdup (_("Unable to mark launcher trusted (executable)")),
				      error->message,
				      NULL,
				      FALSE,
				      GTK_STOCK_CANCEL, RETRY,
				      NULL);
		
		if (response == 0 || response == GTK_RESPONSE_DELETE_EVENT) {
			abort_job (common);
		} else if (response == 1) {
			goto retry;
		} else {
			g_assert_not_reached ();
		}
		
		goto out;
	}
	
	
	if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_UNIX_MODE)) {
		current = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_UNIX_MODE);
		current = current | S_IXGRP | S_IXUSR | S_IXOTH;

		if (!g_file_set_attribute_uint32 (job->file, G_FILE_ATTRIBUTE_UNIX_MODE,
						  current, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
						  common->cancellable, &error))
			{
				g_object_unref (info);
				
				response = run_error (common,
						      g_strdup (_("Unable to mark launcher trusted (executable)")),
						      error->message,
						      NULL,
						      FALSE,
						      GTK_STOCK_CANCEL, RETRY,
						      NULL);
				
				if (response == 0 || response == GTK_RESPONSE_DELETE_EVENT) {
					abort_job (common);
				} else if (response == 1) {
					goto retry;
				} else {
					g_assert_not_reached ();
				}
				
				goto out;
			}
	} 
	g_object_unref (info);

out:
	
	g_io_scheduler_job_send_to_mainloop_async (io_job,
						   mark_trusted_job_done,
						   job,
						   NULL);

	return FALSE;
}