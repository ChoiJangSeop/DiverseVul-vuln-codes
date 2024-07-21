check_exec_for_suid(
    char *filename,
    gboolean verbose)
{
    struct stat stat_buf;
    char *quoted = quote_string(filename);

    if(!stat(filename, &stat_buf)) {
	char *copy_filename;
	char *s;

	if (stat_buf.st_uid != 0 ) {
	    if (verbose)
		g_printf(_("ERROR [%s is not owned by root]\n"), quoted);
	    g_debug("Error: %s is not owned by root", quoted);
	    amfree(quoted);
	    return FALSE;
	}
	if (stat_buf.st_mode & S_IWOTH) {
	    if (verbose)
		g_printf(_("ERROR [%s is writable by everyone]\n"), quoted);
	    g_debug("Error: %s is writable by everyone", quoted);
	    amfree(quoted);
	    return FALSE;
	}
	if (stat_buf.st_mode & S_IWGRP) {
	    if (verbose)
		g_printf(_("ERROR [%s is writable by the group]\n"), quoted);
	    g_debug("Error: %s is writable by the group", quoted);
	    amfree(quoted);
	    return FALSE;
	}
	copy_filename = g_strdup(filename);
	if ((s = strchr(copy_filename, '/'))) {
	    *s = '\0';
	    if (*copy_filename && !check_exec_for_suid(copy_filename, verbose)) {
		amfree(quoted);
		amfree(copy_filename);
		return FALSE;
	    }
	}
	amfree(copy_filename);
    }
    else {
	if (verbose)
	    g_printf(_("ERROR [can not stat %s: %s]\n"), quoted, strerror(errno));
	g_debug("Error: can not stat %s: %s", quoted, strerror(errno));
	amfree(quoted);
	return FALSE;
    }
    amfree(quoted);
    return TRUE;
}