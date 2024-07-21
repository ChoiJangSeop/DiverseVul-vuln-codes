static int sapi_extract_response_code(const char *header_line)
{
	int code = 200;
	const char *ptr;

	for (ptr = header_line; *ptr; ptr++) {
		if (*ptr == ' ' && *(ptr + 1) != ' ') {
			code = atoi(ptr + 1);
			break;
		}
	}
	
	return code;
}