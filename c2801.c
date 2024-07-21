ftp_chmod(ftpbuf_t *ftp, const int mode, const char *filename, const int filename_len)
{
	char *buffer;

	if (ftp == NULL || filename_len <= 0) {
		return 0;
	}

	spprintf(&buffer, 0, "CHMOD %o %s", mode, filename);

	if (!ftp_putcmd(ftp, "SITE", buffer)) {
		efree(buffer);
		return 0;
	}

	efree(buffer);

	if (!ftp_getresp(ftp) || ftp->resp != 200) {
		return 0;
	}
	
	return 1;
}