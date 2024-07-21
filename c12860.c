static Bool locate_js_script(char *path, const char *file_name, const char *file_ext)
{
	u32 len = (u32) strlen(path);
	strcat(path, file_name);
	if (gf_file_exists(path))
		return GF_TRUE;

	if (!file_ext) {
		strcat(path, ".js");
		if (gf_file_exists(path))
			return GF_TRUE;
	}
	path[len] = 0;
	strcat(path, file_name);
	strcat(path, "/init.js");
	if (gf_file_exists(path))
		return GF_TRUE;

	return GF_FALSE;
}