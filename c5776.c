gcab_folder_extract (GCabFolder *self,
                     GFile *path,
                     guint8 res_data,
                     GCabFileCallback file_callback,
                     GFileProgressCallback progress_callback,
                     gpointer callback_data,
                     GCancellable *cancellable,
                     GError **error)
{
    GError *my_error = NULL;
    gboolean success = FALSE;
    g_autoptr(GDataInputStream) data = NULL;
    g_autoptr(GFileOutputStream) out = NULL;
    GSList *f = NULL;
    g_autoptr(GSList) files = NULL;
    cdata_t cdata = { 0, };
    guint32 nubytes = 0;

    /* never loaded from a stream */
    g_assert (self->cfolder != NULL);

    data = g_data_input_stream_new (self->stream);
    g_data_input_stream_set_byte_order (data, G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN);
    g_filter_input_stream_set_close_base_stream (G_FILTER_INPUT_STREAM (data), FALSE);

    if (!g_seekable_seek (G_SEEKABLE (data), self->cfolder->offsetdata, G_SEEK_SET, cancellable, error))
        goto end;

    files = g_slist_sort (g_slist_copy (self->files), (GCompareFunc)sort_by_offset);

    for (f = files; f != NULL; f = f->next) {
        GCabFile *file = f->data;

        if (file_callback && !file_callback (file, callback_data))
            continue;

        g_autofree gchar *fname = g_strdup (gcab_file_get_extract_name (file));
        int i = 0, len = strlen (fname);
        for (i = 0; i < len; i++)
            if (fname[i] == '\\')
                fname[i] = '/';

        g_autoptr(GFile) gfile = g_file_resolve_relative_path (path, fname);

        if (!g_file_has_prefix (gfile, path)) {
            // "Rebase" the file in the given path, to ensure we never escape it
            g_autofree gchar *rawpath = g_file_get_path (gfile);
            if (rawpath != NULL) {
                char *newpath = rawpath;
                while (*newpath != 0 && *newpath == G_DIR_SEPARATOR) {
                    newpath++;
                }
                g_autoptr(GFile) newgfile = g_file_resolve_relative_path (path, newpath);
                g_set_object (&gfile, newgfile);
            }
        }

        g_autoptr(GFile) parent = g_file_get_parent (gfile);

        if (!g_file_make_directory_with_parents (parent, cancellable, &my_error)) {
            if (g_error_matches (my_error, G_IO_ERROR, G_IO_ERROR_EXISTS))
                g_clear_error (&my_error);
            else {
                g_propagate_error (error, my_error);
                goto end;
            }
        }

        g_autoptr(GFileOutputStream) out2 = NULL;
        out2 = g_file_replace (gfile, NULL, FALSE, G_FILE_CREATE_REPLACE_DESTINATION, cancellable, error);
        if (!out2)
            goto end;

        guint32 usize = gcab_file_get_usize (file);
        guint32 uoffset = gcab_file_get_uoffset (file);

        /* let's rewind if need be */
        if (uoffset < nubytes) {
            if (!g_seekable_seek (G_SEEKABLE (data), self->cfolder->offsetdata,
                                  G_SEEK_SET, cancellable, error))
                goto end;
            bzero(&cdata, sizeof(cdata));
            nubytes = 0;
        }

        while (usize > 0) {
            if ((nubytes + cdata.nubytes) <= uoffset) {
                nubytes += cdata.nubytes;
                if (!cdata_read (&cdata, res_data, self->comptype,
                                 data, cancellable, error))
                    goto end;
                continue;
            } else {
                gsize offset = gcab_file_get_uoffset (file) > nubytes ?
                    gcab_file_get_uoffset (file) - nubytes : 0;
                const void *p = &cdata.out[offset];
                gsize count = MIN (usize, cdata.nubytes - offset);
                if (!g_output_stream_write_all (G_OUTPUT_STREAM (out2), p, count,
                                                NULL, cancellable, error))
                    goto end;
                usize -= count;
                uoffset += count;
            }
        }
    }

    success = TRUE;

end:
    cdata_finish (&cdata, NULL);

    return success;
}