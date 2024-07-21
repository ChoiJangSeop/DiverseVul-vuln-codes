nautilus_file_clear_info (NautilusFile *file)
{
	file->details->got_file_info = FALSE;
	if (file->details->get_info_error) {
		g_error_free (file->details->get_info_error);
		file->details->get_info_error = NULL;
	}
	/* Reset to default type, which might be other than unknown for
	   special kinds of files like the desktop or a search directory */
	file->details->type = NAUTILUS_FILE_GET_CLASS (file)->default_file_type;

	if (!file->details->got_custom_display_name) {
		nautilus_file_clear_display_name (file);
	}

	if (!file->details->got_custom_activation_location &&
	    file->details->activation_location != NULL) {
		g_object_unref (file->details->activation_location);
		file->details->activation_location = NULL;
	}
	
	if (file->details->icon != NULL) {
		g_object_unref (file->details->icon);
		file->details->icon = NULL;
	}

	g_free (file->details->thumbnail_path);
	file->details->thumbnail_path = NULL;
	file->details->thumbnailing_failed = FALSE;
	
	file->details->is_launcher = FALSE;
	file->details->is_foreign_link = FALSE;
	file->details->is_symlink = FALSE;
	file->details->is_hidden = FALSE;
	file->details->is_backup = FALSE;
	file->details->is_mountpoint = FALSE;
	file->details->uid = -1;
	file->details->gid = -1;
	file->details->can_read = TRUE;
	file->details->can_write = TRUE;
	file->details->can_execute = TRUE;
	file->details->can_delete = TRUE;
	file->details->can_trash = TRUE;
	file->details->can_rename = TRUE;
	file->details->can_mount = FALSE;
	file->details->can_unmount = FALSE;
	file->details->can_eject = FALSE;
	file->details->has_permissions = FALSE;
	file->details->permissions = 0;
	file->details->size = -1;
	file->details->sort_order = 0;
	file->details->mtime = 0;
	file->details->atime = 0;
	file->details->ctime = 0;
	g_free (file->details->symlink_name);
	file->details->symlink_name = NULL;
	eel_ref_str_unref (file->details->mime_type);
	file->details->mime_type = NULL;
	g_free (file->details->selinux_context);
	file->details->selinux_context = NULL;
	g_free (file->details->description);
	file->details->description = NULL;

	eel_ref_str_unref (file->details->filesystem_id);
	file->details->filesystem_id = NULL;
}