nautilus_file_operations_copy (GList *files,
			       GArray *relative_item_points,
			       GFile *target_dir,
			       GtkWindow *parent_window,
			       NautilusCopyCallback  done_callback,
			       gpointer done_callback_data)
{
	CopyMoveJob *job;

	job = op_job_new (CopyMoveJob, parent_window);
	job->done_callback = done_callback;
	job->done_callback_data = done_callback_data;
	job->files = eel_g_object_list_copy (files);
	job->destination = g_object_ref (target_dir);
	if (relative_item_points != NULL &&
	    relative_item_points->len > 0) {
		job->icon_positions =
			g_memdup (relative_item_points->data,
				  sizeof (GdkPoint) * relative_item_points->len);
		job->n_icon_positions = relative_item_points->len;
	}
	job->debuting_files = g_hash_table_new_full (g_file_hash, (GEqualFunc)g_file_equal, g_object_unref, NULL);

	g_io_scheduler_push_job (copy_job,
			   job,
			   NULL, /* destroy notify */
			   0,
			   job->common.cancellable);
}