is_link_data (const char *file_contents, int file_size)
{
	char *mimetype;
	gboolean res;

	mimetype = g_content_type_guess (NULL, file_contents, file_size, NULL);
	res =  is_link_mime_type (mimetype);
	g_free (mimetype);
	return res;
}