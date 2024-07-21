ftp_nb_put(ftpbuf_t *ftp, const char *path, php_stream *instream, ftptype_t type, long startpos TSRMLS_DC)
{
	databuf_t		*data = NULL;
	char			arg[11];

	if (ftp == NULL) {
		return 0;
	}
	if (!ftp_type(ftp, type)) {
		goto bail;
	}
	if ((data = ftp_getdata(ftp TSRMLS_CC)) == NULL) {
		goto bail;
	}
	if (startpos > 0) {
		snprintf(arg, sizeof(arg), "%ld", startpos);
		if (!ftp_putcmd(ftp, "REST", arg)) {
			goto bail;
		}
		if (!ftp_getresp(ftp) || (ftp->resp != 350)) {
			goto bail;
		}
	}

	if (!ftp_putcmd(ftp, "STOR", path)) {
		goto bail;
	}
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125)) {
		goto bail;
	}
	if ((data = data_accept(data, ftp TSRMLS_CC)) == NULL) { 
		goto bail;
	}
	ftp->data = data;
	ftp->stream = instream;
	ftp->lastch = 0;
	ftp->nb = 1;

	return (ftp_nb_continue_write(ftp TSRMLS_CC));

bail:
	ftp->data = data_close(ftp, data);
	return PHP_FTP_FAILED;
}