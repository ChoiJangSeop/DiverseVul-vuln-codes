copy_job (GIOSchedulerJob *io_job,
	  GCancellable *cancellable,
	  gpointer user_data)
{
	CopyMoveJob *job;
	CommonJob *common;
	SourceInfo source_info;
	TransferInfo transfer_info;
	char *dest_fs_id;
	GFile *dest;

	job = user_data;
	common = &job->common;
	common->io_job = io_job;

	dest_fs_id = NULL;
	
	nautilus_progress_info_start (job->common.progress);
	
	scan_sources (job->files,
		      &source_info,
		      common,
		      OP_KIND_COPY);
	if (job_aborted (common)) {
		goto aborted;
	}

	if (job->destination) {
		dest = g_object_ref (job->destination);
	} else {
		/* Duplication, no dest,
		 * use source for free size, etc
		 */
		dest = g_file_get_parent (job->files->data);
	}
	
	verify_destination (&job->common,
			    dest,
			    &dest_fs_id,
			    source_info.num_bytes);
	g_object_unref (dest);
	if (job_aborted (common)) {
		goto aborted;
	}

	g_timer_start (job->common.time);
	
	memset (&transfer_info, 0, sizeof (transfer_info));
	copy_files (job,
		    dest_fs_id,
		    &source_info, &transfer_info);
	
 aborted:
	
	g_free (dest_fs_id);
	
	g_io_scheduler_job_send_to_mainloop_async (io_job,
						   copy_job_done,
						   job,
						   NULL);

	return FALSE;
}