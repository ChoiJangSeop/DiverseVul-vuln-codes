gxps_archive_read_entry (GXPSArchive *archive,
			 const gchar *path,
			 guchar     **buffer,
			 gsize       *bytes_read,
			 GError     **error)
{
	GInputStream *stream;
	gssize        entry_size;
	gboolean      retval;

	stream = gxps_archive_open (archive, path);
	if (!stream)
		/* TODO: Error */
		return FALSE;

	entry_size = archive_entry_size (GXPS_ARCHIVE_INPUT_STREAM (stream)->entry);
	if (entry_size <= 0) {
		gssize bytes;
		guchar buf[BUFFER_SIZE];
		guint  buffer_size = BUFFER_SIZE * 4;

		/* In some cases, I don't know why, archive_entry_size() returns 0,
		 * but the entry can be read, so let's try here.
		 */
		*bytes_read = 0;
		*buffer = g_malloc (buffer_size);
		do {
			bytes = g_input_stream_read (stream, &buf, BUFFER_SIZE, NULL, error);
			if (*error != NULL) {
				g_free (*buffer);
				g_object_unref (stream);

				return FALSE;
			}

			if (*bytes_read + bytes > buffer_size) {
				buffer_size += BUFFER_SIZE * 4;
				*buffer = g_realloc (*buffer, buffer_size);
			}
			memcpy (*buffer + *bytes_read, buf, bytes);
			*bytes_read += bytes;
		} while (bytes > 0);

		g_object_unref (stream);

		if (*bytes_read == 0) {
			/* TODO: Error */
			g_free (*buffer);
			return FALSE;
		}

		return TRUE;
	}

	*buffer = g_malloc (entry_size);
	retval = g_input_stream_read_all (stream,
					  *buffer, entry_size,
					  bytes_read, NULL,
					  error);
	if (!retval)
		g_free (*buffer);

	g_object_unref (stream);

	return retval;
}