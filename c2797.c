ftp_get(ftpbuf_t *ftp, php_stream *outstream, const char *path, ftptype_t type, long resumepos TSRMLS_DC)
{
	databuf_t		*data = NULL;
	int			lastch;
	size_t			rcvd;
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
	
	ftp->data = data;

	if (resumepos > 0) {
		snprintf(arg, sizeof(arg), "%ld", resumepos);
		if (!ftp_putcmd(ftp, "REST", arg)) {
			goto bail;
		}
		if (!ftp_getresp(ftp) || (ftp->resp != 350)) {
			goto bail;
		}
	}

	if (!ftp_putcmd(ftp, "RETR", path)) {
		goto bail;
	}
	if (!ftp_getresp(ftp) || (ftp->resp != 150 && ftp->resp != 125)) {
		goto bail;
	}

	if ((data = data_accept(data, ftp TSRMLS_CC)) == NULL) {
		goto bail;
	}

	lastch = 0;
	while ((rcvd = my_recv(ftp, data->fd, data->buf, FTP_BUFSIZE))) {
		if (rcvd == -1) {
			goto bail;
		}

		if (type == FTPTYPE_ASCII) {
#ifndef PHP_WIN32
			char *s;
#endif
			char *ptr = data->buf;
			char *e = ptr + rcvd;
			/* logic depends on the OS EOL
			 * Win32 -> \r\n
			 * Everything Else \n
			 */
#ifdef PHP_WIN32
			php_stream_write(outstream, ptr, (e - ptr));
			ptr = e;
#else
			while (e > ptr && (s = memchr(ptr, '\r', (e - ptr)))) {
				php_stream_write(outstream, ptr, (s - ptr));
				if (*(s + 1) == '\n') {
					s++;
					php_stream_putc(outstream, '\n');
				}
				ptr = s + 1;
			}
#endif
			if (ptr < e) {
				php_stream_write(outstream, ptr, (e - ptr));
			}
		} else if (rcvd != php_stream_write(outstream, data->buf, rcvd)) {
			goto bail;
		}
	}

	ftp->data = data = data_close(ftp, data);

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250)) {
		goto bail;
	}

	return 1;
bail:
	ftp->data = data_close(ftp, data);
	return 0;
}