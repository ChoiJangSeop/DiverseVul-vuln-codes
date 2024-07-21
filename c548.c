gdm_session_settings_load (GdmSessionSettings  *settings,
                           const char          *home_directory,
                           GError             **error)
{
        GKeyFile *key_file;
        GError   *load_error;
        gboolean  is_loaded;
        char     *session_name;
        char     *language_name;
        char     *layout_name;
        char     *filename;

        g_return_val_if_fail (settings != NULL, FALSE);
        g_return_val_if_fail (home_directory != NULL, FALSE);
        g_return_val_if_fail (!gdm_session_settings_is_loaded (settings), FALSE);
        filename = g_build_filename (home_directory, ".dmrc", NULL);

        is_loaded = FALSE;
        key_file = g_key_file_new ();

        load_error = NULL;
        if (!g_key_file_load_from_file (key_file, filename,
                                        G_KEY_FILE_NONE, &load_error)) {
                g_propagate_error (error, load_error);
                goto out;
        }

        session_name = g_key_file_get_string (key_file, "Desktop", "Session",
                                              &load_error);

        if (session_name != NULL) {
                gdm_session_settings_set_session_name (settings, session_name);
                g_free (session_name);
        } else if (g_error_matches (load_error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
                g_error_free (load_error);
                load_error = NULL;
        } else {
                g_propagate_error (error, load_error);
                goto out;
        }

        language_name = g_key_file_get_string (key_file, "Desktop", "Language",
                                               &load_error);

        if (language_name != NULL) {
                gdm_session_settings_set_language_name (settings, language_name);
                g_free (language_name);
        } else if (g_error_matches (load_error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
                g_error_free (load_error);
                load_error = NULL;
        } else {
                g_propagate_error (error, load_error);
                goto out;
        }

        layout_name = g_key_file_get_string (key_file, "Desktop", "Layout",
                                             &load_error);

        if (layout_name != NULL) {
                gdm_session_settings_set_layout_name (settings, layout_name);
                g_free (layout_name);
        } else if (g_error_matches (load_error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
                g_error_free (load_error);
                load_error = NULL;
        } else {
                g_propagate_error (error, load_error);
                goto out;
        }

        is_loaded = TRUE;
out:
        g_key_file_free (key_file);
        g_free (filename);

        return is_loaded;
}