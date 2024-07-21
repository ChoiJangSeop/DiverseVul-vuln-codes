static int is_userinfo_valid(const char *str, size_t len)
{
	char *valid = "-._~!$&'()*+,;=:";
	char *p = str;
	while (p - str < len) {
		if (isalpha(*p) || isdigit(*p) || strchr(valid, *p)) {
			p++;
		} else if (*p == '%' && p - str <= len - 3 && isdigit(*(p+1)) && isxdigit(*(p+2))) {
			p += 3;
		} else {
			return 0;
		}
	}
	return 1;
}