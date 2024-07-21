append_acs(string_desc * dst, int code, char *src)
{
    if (VALID_STRING(src) && strlen(src) == 1) {
	append_acs0(dst, code, *src);
    }
}