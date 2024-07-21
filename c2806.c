ftp_syst(ftpbuf_t *ftp)
{
	char *syst, *end;

	if (ftp == NULL) {
		return NULL;
	}

	/* default to cached value */
	if (ftp->syst) {
		return ftp->syst;
	}
	if (!ftp_putcmd(ftp, "SYST", NULL)) {
		return NULL;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 215) { 
		return NULL;
	}
	syst = ftp->inbuf;
	while (*syst == ' ') {
		syst++;
	}
	if ((end = strchr(syst, ' '))) {
		*end = 0;
	}
	ftp->syst = estrdup(syst);
	if (end) {
		*end = ' ';
	}
	return ftp->syst;
}