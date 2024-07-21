ftp_alloc(ftpbuf_t *ftp, const long size, char **response)
{
	char buffer[64];

	if (ftp == NULL || size <= 0) {
		return 0;
	}

	snprintf(buffer, sizeof(buffer) - 1, "%ld", size);
    
	if (!ftp_putcmd(ftp, "ALLO", buffer)) {
		return 0;
	}

	if (!ftp_getresp(ftp)) {
		return 0;
	}

	if (response) {
		*response = estrdup(ftp->inbuf);
	}

	if (ftp->resp < 200 || ftp->resp >= 300) {
		return 0;
	}

	return 1;	
}