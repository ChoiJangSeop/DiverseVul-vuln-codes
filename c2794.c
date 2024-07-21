ftp_putcmd(ftpbuf_t *ftp, const char *cmd, const char *args)
{
	int		size;
	char		*data;

	if (strpbrk(cmd, "\r\n")) {
		return 0;
	} 
	/* build the output buffer */
	if (args && args[0]) {
		/* "cmd args\r\n\0" */
		if (strlen(cmd) + strlen(args) + 4 > FTP_BUFSIZE) {
			return 0;
		}
		if (strpbrk(args, "\r\n")) {
			return 0;
		}
		size = slprintf(ftp->outbuf, sizeof(ftp->outbuf), "%s %s\r\n", cmd, args);
	} else {
		/* "cmd\r\n\0" */
		if (strlen(cmd) + 3 > FTP_BUFSIZE) {
			return 0;
		}
		size = slprintf(ftp->outbuf, sizeof(ftp->outbuf), "%s\r\n", cmd);
	}

	data = ftp->outbuf;

	/* Clear the extra-lines buffer */
	ftp->extra = NULL;

	if (my_send(ftp, ftp->fd, data, size) != size) {
		return 0;
	}
	return 1;
}