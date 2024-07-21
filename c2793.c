ftp_reinit(ftpbuf_t *ftp)
{
	if (ftp == NULL) {
		return 0;
	}	

	ftp_gc(ftp);

	ftp->nb = 0;

	if (!ftp_putcmd(ftp, "REIN", NULL)) {
		return 0;
	}
	if (!ftp_getresp(ftp) || ftp->resp != 220) {
		return 0;
	}

	return 1;
}