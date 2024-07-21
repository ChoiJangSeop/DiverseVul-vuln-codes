nautilus_file_set_display_name (NautilusFile *file,
				const char *display_name,
				const char *edit_name,
				gboolean custom)
{
	gboolean changed;

	if (display_name == NULL || *display_name == 0) {
		return FALSE;
	}
	
	if (!custom && file->details->got_custom_display_name) {
		return FALSE;
	}

	if (custom && display_name == NULL) {
		/* We're re-setting a custom display name, invalidate it if
		   we already set it so that the old one is re-read */
		if (file->details->got_custom_display_name) {
			file->details->got_custom_display_name = FALSE;
			nautilus_file_invalidate_attributes (file,
							     NAUTILUS_FILE_ATTRIBUTE_INFO);
		}
		return FALSE;
	}

	if (edit_name == NULL) {
		edit_name = display_name;
	}
	    
	changed = FALSE;
	
	if (eel_strcmp (eel_ref_str_peek (file->details->display_name), display_name) != 0) {
		changed = TRUE;
		
		eel_ref_str_unref (file->details->display_name);
		
		if (eel_strcmp (eel_ref_str_peek (file->details->name), display_name) == 0) {
			file->details->display_name = eel_ref_str_ref (file->details->name);
		} else {
			file->details->display_name = eel_ref_str_new (display_name);
		}
		
		g_free (file->details->display_name_collation_key);
		file->details->display_name_collation_key = g_utf8_collate_key_for_filename (display_name, -1);
	}

	if (eel_strcmp (eel_ref_str_peek (file->details->edit_name), edit_name) != 0) {
		changed = TRUE;
		
		eel_ref_str_unref (file->details->edit_name);
		if (eel_strcmp (eel_ref_str_peek (file->details->display_name), edit_name) == 0) {
			file->details->edit_name = eel_ref_str_ref (file->details->display_name);
		} else {
			file->details->edit_name = eel_ref_str_new (edit_name);
		}
	}
	
	file->details->got_custom_display_name = custom;
	return changed;
}