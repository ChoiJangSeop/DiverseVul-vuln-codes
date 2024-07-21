ftp_put(ftpbuf_t *ftp, const char *path, php_stream *instream, ftptype_t type, long startpos TSRMLS_DC)
{
	databuf_t		*data = NULL;
	long			size;
	char			*ptr;
	int			ch;
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

	size = 0;
	ptr = data->buf;
	while (!php_stream_eof(instream) && (ch = php_stream_getc(instream))!=EOF) {
		/* flush if necessary */
		if (FTP_BUFSIZE - size < 2) {
			if (my_send(ftp, data->fd, data->buf, size) != size) {
				goto bail;
			}
			ptr = data->buf;
			size = 0;
		}

		if (ch == '\n' && type == FTPTYPE_ASCII) {
			*ptr++ = '\r';
			size++;
		}

		*ptr++ = ch;
		size++;
	}

	if (size && my_send(ftp, data->fd, data->buf, size) != size) {
		goto bail;
	}
	ftp->data = data = data_close(ftp, data);

	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250 && ftp->resp != 200)) {
		goto bail;
	}
	return 1;
bail:
	ftp->data = data_close(ftp, data);
	return 0;
}