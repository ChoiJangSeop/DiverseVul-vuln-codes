archive_wstring_append_from_mbs(struct archive_wstring *dest,
    const char *p, size_t len)
{
	return archive_wstring_append_from_mbs_in_codepage(dest, p, len, NULL);
}