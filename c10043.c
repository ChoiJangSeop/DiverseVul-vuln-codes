autoar_extractor_step_extract (AutoarExtractor *self) {
  /* Step 3: Extract files
   * We have to re-open the archive to extract files
   */

  struct archive *a;
  struct archive_entry *entry;

  int r;

  g_debug ("autoar_extractor_step_extract: called");

  r = libarchive_create_read_object (self->use_raw_format, self, &a);
  if (r != ARCHIVE_OK) {
    if (self->error == NULL) {
      self->error =
        autoar_common_g_error_new_a (a, self->source_basename);
    }
    archive_read_free (a);
    return;
  }

  while ((r = archive_read_next_header (a, &entry)) == ARCHIVE_OK) {
    const char *pathname;
    const char *hardlink;
    g_autoptr (GFile) extracted_filename = NULL;
    g_autoptr (GFile) hardlink_filename = NULL;
    AutoarConflictAction action;
    gboolean file_conflict;

    if (g_cancellable_is_cancelled (self->cancellable)) {
      archive_read_free (a);
      return;
    }

    pathname = archive_entry_pathname (entry);
    hardlink = archive_entry_hardlink (entry);

    extracted_filename =
      autoar_extractor_do_sanitize_pathname (self, pathname);

    if (hardlink != NULL) {
      hardlink_filename =
        autoar_extractor_do_sanitize_pathname (self, hardlink);
    }

    /* Attempt to solve any name conflict before doing any operations */
    file_conflict = autoar_extractor_check_file_conflict (extracted_filename,
                                                          archive_entry_filetype (entry));
    while (file_conflict) {
      GFile *new_extracted_filename = NULL;

      action = autoar_extractor_signal_conflict (self,
                                                 extracted_filename,
                                                 &new_extracted_filename);

      switch (action) {
        case AUTOAR_CONFLICT_OVERWRITE:
          break;
        case AUTOAR_CONFLICT_CHANGE_DESTINATION:
          g_assert_nonnull (new_extracted_filename);
          g_clear_object (&extracted_filename);
          extracted_filename = new_extracted_filename;
          break;
        case AUTOAR_CONFLICT_SKIP:
          archive_read_data_skip (a);
          break;
        default:
          g_assert_not_reached ();
          break;
      }

      if (action != AUTOAR_CONFLICT_CHANGE_DESTINATION) {
        break;
      }

      file_conflict = autoar_extractor_check_file_conflict (extracted_filename,
                                                            archive_entry_filetype (entry));
    }

    if (file_conflict && action == AUTOAR_CONFLICT_SKIP) {
      continue;
    }

    autoar_extractor_do_write_entry (self, a, entry,
                                     extracted_filename, hardlink_filename);

    if (self->error != NULL) {
      archive_read_free (a);
      return;
    }

    self->completed_files++;
    autoar_extractor_signal_progress (self);
  }

  if (r != ARCHIVE_EOF) {
    if (self->error == NULL) {
      self->error =
        autoar_common_g_error_new_a (a, self->source_basename);
    }
    archive_read_free (a);
    return;
  }

  archive_read_free (a);
}