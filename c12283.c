bool samba_private_attr_name(const char *unix_ea_name)
{
	static const char * const prohibited_ea_names[] = {
		SAMBA_POSIX_INHERITANCE_EA_NAME,
		SAMBA_XATTR_DOS_ATTRIB,
		SAMBA_XATTR_MARKER,
		XATTR_NTACL_NAME,
		NULL
	};

	int i;

	for (i = 0; prohibited_ea_names[i]; i++) {
		if (strequal( prohibited_ea_names[i], unix_ea_name))
			return true;
	}
	if (strncasecmp_m(unix_ea_name, SAMBA_XATTR_DOSSTREAM_PREFIX,
			strlen(SAMBA_XATTR_DOSSTREAM_PREFIX)) == 0) {
		return true;
	}
	return false;
}