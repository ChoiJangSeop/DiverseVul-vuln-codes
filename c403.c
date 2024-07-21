copy_move_file (CopyMoveJob *copy_job,
		GFile *src,
		GFile *dest_dir,
		gboolean same_fs,
		gboolean unique_names,
		char **dest_fs_type,
		SourceInfo *source_info,
		TransferInfo *transfer_info,
		GHashTable *debuting_files,
		GdkPoint *position,
		gboolean overwrite,
		gboolean *skipped_file,
		gboolean readonly_source_fs)
{
	GFile *dest, *new_dest;
	GError *error;
	GFileCopyFlags flags;
	char *primary, *secondary, *details;
	int response;
	ProgressData pdata;
	gboolean would_recurse, is_merge;
	CommonJob *job;
	gboolean res;
	int unique_name_nr;
	gboolean handled_invalid_filename;

	job = (CommonJob *)copy_job;
	
	if (should_skip_file (job, src)) {
		*skipped_file = TRUE;
		return;
	}

	unique_name_nr = 1;

	/* another file in the same directory might have handled the invalid
	 * filename condition for us
	 */
	handled_invalid_filename = *dest_fs_type != NULL;

	if (unique_names) {
		dest = get_unique_target_file (src, dest_dir, same_fs, *dest_fs_type, unique_name_nr++);
	} else {
		dest = get_target_file (src, dest_dir, *dest_fs_type, same_fs);
	}


	/* Don't allow recursive move/copy into itself.  
	 * (We would get a file system error if we proceeded but it is nicer to 
	 * detect and report it at this level) */
	if (test_dir_is_parent (dest_dir, src)) {
		if (job->skip_all_error) {
			g_error_free (error);
			goto out;
		}
		
		/*  the run_warning() frees all strings passed in automatically  */
		primary = copy_job->is_move ? g_strdup (_("You cannot move a folder into itself."))
					    : g_strdup (_("You cannot copy a folder into itself."));
		secondary = g_strdup (_("The destination folder is inside the source folder."));
		
		response = run_warning (job,
					primary,
					secondary,
					NULL,
					(source_info->num_files - transfer_info->num_files) > 1,
					GTK_STOCK_CANCEL, SKIP_ALL, SKIP,
					NULL);

		if (response == 0 || response == GTK_RESPONSE_DELETE_EVENT) {
			abort_job (job);
		} else if (response == 1) { /* skip all */
			job->skip_all_error = TRUE;
		} else if (response == 2) { /* skip */
			/* do nothing */
		} else {
			g_assert_not_reached ();
		}

		goto out;
	}

	/* Don't allow copying over the source or one of the parents of the source.
	 */
	if (test_dir_is_parent (src, dest)) {
		if (job->skip_all_error) {
			g_error_free (error);
			goto out;
		}
		
		/*  the run_warning() frees all strings passed in automatically  */
		primary = copy_job->is_move ? g_strdup (_("You cannot move a file over itself."))
					    : g_strdup (_("You cannot copy a file over itself."));
		secondary = g_strdup (_("The source file would be overwritten by the destination."));
		
		response = run_warning (job,
					primary,
					secondary,
					NULL,
					(source_info->num_files - transfer_info->num_files) > 1,
					GTK_STOCK_CANCEL, SKIP_ALL, SKIP,
					NULL);

		if (response == 0 || response == GTK_RESPONSE_DELETE_EVENT) {
			abort_job (job);
		} else if (response == 1) { /* skip all */
			job->skip_all_error = TRUE;
		} else if (response == 2) { /* skip */
			/* do nothing */
		} else {
			g_assert_not_reached ();
		}

		goto out;
	}

	
 retry:
	
	error = NULL;
	flags = G_FILE_COPY_NOFOLLOW_SYMLINKS;
	if (overwrite) {
		flags |= G_FILE_COPY_OVERWRITE;
	}
	if (readonly_source_fs) {
		flags |= G_FILE_COPY_TARGET_DEFAULT_PERMS;
	}

	pdata.job = copy_job;
	pdata.last_size = 0;
	pdata.source_info = source_info;
	pdata.transfer_info = transfer_info;

	if (copy_job->is_move) {
		res = g_file_move (src, dest,
				   flags,
				   job->cancellable,
				   copy_file_progress_callback,
				   &pdata,
				   &error);
	} else {
		res = g_file_copy (src, dest,
				   flags,
				   job->cancellable,
				   copy_file_progress_callback,
				   &pdata,
				   &error);
	}
	
	if (res) {
		transfer_info->num_files ++;
		report_copy_progress (copy_job, source_info, transfer_info);

		if (debuting_files) {
			if (copy_job->is_move) {
				nautilus_file_changes_queue_schedule_metadata_move (src, dest);
			} else {
				nautilus_file_changes_queue_schedule_metadata_copy (src, dest);
			}
			if (position) {
				nautilus_file_changes_queue_schedule_position_set (dest, *position, job->screen_num);
			} else {
				nautilus_file_changes_queue_schedule_position_remove (dest);
			}
			
			g_hash_table_replace (debuting_files, g_object_ref (dest), GINT_TO_POINTER (TRUE));
		}
		if (copy_job->is_move) {
			nautilus_file_changes_queue_file_moved (src, dest);
		} else {
			nautilus_file_changes_queue_file_added (dest);
		}
		g_object_unref (dest);
		return;
	}

	if (!handled_invalid_filename &&
	    IS_IO_ERROR (error, INVALID_FILENAME)) {
		handled_invalid_filename = TRUE;

		g_assert (*dest_fs_type == NULL);
		*dest_fs_type = query_fs_type (dest_dir, job->cancellable);

		if (unique_names) {
			new_dest = get_unique_target_file (src, dest_dir, same_fs, *dest_fs_type, unique_name_nr);
		} else {
			new_dest = get_target_file (src, dest_dir, *dest_fs_type, same_fs);
		}

		if (!g_file_equal (dest, new_dest)) {
			g_object_unref (dest);
			dest = new_dest;

			g_error_free (error);
			goto retry;
		} else {
			g_object_unref (new_dest);
		}
	}

	/* Conflict */
	if (!overwrite &&
	    IS_IO_ERROR (error, EXISTS)) {
		gboolean is_merge;

		if (unique_names) {
			g_object_unref (dest);
			dest = get_unique_target_file (src, dest_dir, same_fs, *dest_fs_type, unique_name_nr++);
			g_error_free (error);
			goto retry;
		}
		
		is_merge = FALSE;
		if (is_dir (dest)) {
			if (is_dir (src)) {
				is_merge = TRUE;
				primary = f (_("A folder named \"%B\" already exists.  Do you want to merge the source folder?"), 
					     dest);
				secondary = f (_("The source folder already exists in \"%B\".  "
						 "Merging will ask for confirmation before replacing any files in the folder that conflict with the files being copied."), 
					       dest_dir);
				
			} else {
				primary = f (_("A folder named \"%B\" already exists.  Do you want to replace it?"), 
							    dest);
				secondary = f (_("The folder already exists in \"%F\".  "
						 "Replacing it will remove all files in the folder."), 
					       dest_dir);
			}
		} else {
			primary = f (_("A file named \"%B\" already exists.  Do you want to replace it?"), 
				     dest);
			secondary = f (_("The file already exists in \"%F\".  "
					 "Replacing it will overwrite its content."), 
				       dest_dir);
		}

		if ((is_merge && job->merge_all) ||
		    (!is_merge && job->replace_all)) {
			g_free (primary);
			g_free (secondary);
			g_error_free (error);
			
			overwrite = TRUE;
			goto retry;
		}

		if (job->skip_all_conflict) {
			g_free (primary);
			g_free (secondary);
			g_error_free (error);
			
			goto out;
		}
		
		response = run_warning (job,
					primary,
					secondary,
					NULL,
					(source_info->num_files - transfer_info->num_files) > 1,
					GTK_STOCK_CANCEL,
					SKIP_ALL,
					is_merge?MERGE_ALL:REPLACE_ALL,
					SKIP,
					is_merge?MERGE:REPLACE,
					NULL);
		
		g_error_free (error);
		
		if (response == 0 || response == GTK_RESPONSE_DELETE_EVENT) {
			abort_job (job);
		} else if (response == 1 || response == 3) { /* skip all / skip */
			if (response == 1) {
				job->skip_all_conflict = TRUE;
			}
		} else if (response == 2 || response == 4) { /* merge/replace all  / merge/replace*/
			if (response == 2) {
				if (is_merge) {
					job->merge_all = TRUE;
				} else {
					job->replace_all = TRUE;
				}
			}
			overwrite = TRUE;
			goto retry;
		} else {
			g_assert_not_reached ();
		}
	}
	
	else if (overwrite &&
		 IS_IO_ERROR (error, IS_DIRECTORY)) {

		g_error_free (error);
		
		if (remove_target_recursively (job, src, dest, dest)) {
			goto retry;
		}
	}
	
	/* Needs to recurse */
	else if (IS_IO_ERROR (error, WOULD_RECURSE) ||
		 IS_IO_ERROR (error, WOULD_MERGE)) {
		is_merge = error->code == G_IO_ERROR_WOULD_MERGE;
		would_recurse = error->code == G_IO_ERROR_WOULD_RECURSE;
		g_error_free (error);

		if (overwrite && would_recurse) {
			error = NULL;
			
			/* Copying a dir onto file, first remove the file */
			if (!g_file_delete (dest, job->cancellable, &error) &&
			    !IS_IO_ERROR (error, NOT_FOUND)) {
				if (job->skip_all_error) {
					g_error_free (error);
					goto out;
				}
				if (copy_job->is_move) {
					primary = f (_("Error while moving \"%B\"."), src);
				} else {
					primary = f (_("Error while copying \"%B\"."), src);
				}
				secondary = f (_("Could not remove the already existing file with the same name in %F."), dest_dir);
				details = error->message;

				/* setting TRUE on show_all here, as we could have
				 * another error on the same file later.
				 */
				response = run_warning (job,
							primary,
							secondary,
							details,
							TRUE,
							GTK_STOCK_CANCEL, SKIP_ALL, SKIP,
							NULL);
				
				g_error_free (error);
				
				if (response == 0 || response == GTK_RESPONSE_DELETE_EVENT) {
					abort_job (job);
				} else if (response == 1) { /* skip all */
					job->skip_all_error = TRUE;
				} else if (response == 2) { /* skip */
					/* do nothing */
				} else {
					g_assert_not_reached ();
				}
				goto out;
				
			}
			if (error) {
				g_error_free (error);
				error = NULL;
			}
			if (debuting_files) { /* Only remove metadata for toplevel items */
				nautilus_file_changes_queue_schedule_metadata_remove (dest);
			}
			nautilus_file_changes_queue_file_removed (dest);
		}

		if (is_merge) {
			/* On merge we now write in the target directory, which may not
			   be in the same directory as the source, even if the parent is
			   (if the merged directory is a mountpoint). This could cause
			   problems as we then don't transcode filenames.
			   We just set same_fs to FALSE which is safe but a bit slower. */
			same_fs = FALSE;
		}
		
		if (!copy_move_directory (copy_job, src, &dest, same_fs,
					  would_recurse, dest_fs_type,
					  source_info, transfer_info,
					  debuting_files, skipped_file,
					  readonly_source_fs)) {
			/* destination changed, since it was an invalid file name */
			g_assert (*dest_fs_type != NULL);
			handled_invalid_filename = TRUE;
			goto retry;
		}

		g_object_unref (dest);
		return;
	}
	
	else if (IS_IO_ERROR (error, CANCELLED)) {
		g_error_free (error);
	}
	
	/* Other error */
	else {
		if (job->skip_all_error) {
			g_error_free (error);
			goto out;
		}
		primary = f (_("Error while copying \"%B\"."), src);
		secondary = f (_("There was an error copying the file into %F."), dest_dir);
		details = error->message;
		
		response = run_warning (job,
					primary,
					secondary,
					details,
					(source_info->num_files - transfer_info->num_files) > 1,
					GTK_STOCK_CANCEL, SKIP_ALL, SKIP,
					NULL);

		g_error_free (error);
		
		if (response == 0 || response == GTK_RESPONSE_DELETE_EVENT) {
			abort_job (job);
		} else if (response == 1) { /* skip all */
			job->skip_all_error = TRUE;
		} else if (response == 2) { /* skip */
			/* do nothing */
		} else {
			g_assert_not_reached ();
		}
	}
 out:
	*skipped_file = TRUE; /* Or aborted, but same-same */
	g_object_unref (dest);
}