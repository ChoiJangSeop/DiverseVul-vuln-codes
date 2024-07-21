copy_job_done (gpointer user_data)
{
	CopyMoveJob *job;

	job = user_data;
	if (job->done_callback) {
		job->done_callback (job->debuting_files, job->done_callback_data);
	}

	eel_g_object_list_free (job->files);
	if (job->destination) {
		g_object_unref (job->destination);
	}
	g_hash_table_unref (job->debuting_files);
	g_free (job->icon_positions);
	
	finalize_common ((CommonJob *)job);

	nautilus_file_changes_consume_changes (TRUE);
	return FALSE;
}