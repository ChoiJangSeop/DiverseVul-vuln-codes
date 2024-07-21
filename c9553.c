archive_string_append_from_wcs(struct archive_string *as,
    const wchar_t *w, size_t len)
{
	return archive_string_append_from_wcs_in_codepage(as, w, len, NULL);
}