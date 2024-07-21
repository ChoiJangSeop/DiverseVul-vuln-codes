ftp_nb_continue_write(ftpbuf_t *ftp TSRMLS_DC)
{
	long			size;
	char			*ptr;
	int 			ch;

	/* check if we can write more data */
	if (!data_writeable(ftp, ftp->data->fd)) {
		return PHP_FTP_MOREDATA;
	}

	size = 0;
	ptr = ftp->data->buf;
	while (!php_stream_eof(ftp->stream) && (ch = php_stream_getc(ftp->stream)) != EOF) {

		if (ch == '\n' && ftp->type == FTPTYPE_ASCII) {
			*ptr++ = '\r';
			size++;
		}

		*ptr++ = ch;
		size++;

		/* flush if necessary */
		if (FTP_BUFSIZE - size < 2) {
			if (my_send(ftp, ftp->data->fd, ftp->data->buf, size) != size) {
				goto bail;
			}
			return PHP_FTP_MOREDATA;
		}
	}

	if (size && my_send(ftp, ftp->data->fd, ftp->data->buf, size) != size) {
		goto bail;
	}
	ftp->data = data_close(ftp, ftp->data);
 
	if (!ftp_getresp(ftp) || (ftp->resp != 226 && ftp->resp != 250)) {
		goto bail;
	}
	ftp->nb = 0;
	return PHP_FTP_FINISHED;
bail:
	ftp->data = data_close(ftp, ftp->data);
	ftp->nb = 0;
	return PHP_FTP_FAILED;
}