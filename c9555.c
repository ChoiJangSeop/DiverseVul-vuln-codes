archive_string_append_from_wcs(struct archive_string *as,
    const wchar_t *w, size_t len)
{
	(void)as;/* UNUSED */
	(void)w;/* UNUSED */
	(void)len;/* UNUSED */
	errno = ENOSYS;
	return (-1);
}