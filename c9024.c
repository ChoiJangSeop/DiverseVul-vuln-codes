static int multipart_buffer_read(multipart_buffer *self, char *buf, size_t bytes, int *end)
{
	size_t len, max;
	char *bound;

	/* fill buffer if needed */
	if (bytes > (size_t)self->bytes_in_buffer) {
		fill_buffer(self);
	}

	/* look for a potential boundary match, only read data up to that point */
	if ((bound = php_ap_memstr(self->buf_begin, self->bytes_in_buffer, self->boundary_next, self->boundary_next_len, 1))) {
		max = bound - self->buf_begin;
		if (end && php_ap_memstr(self->buf_begin, self->bytes_in_buffer, self->boundary_next, self->boundary_next_len, 0)) {
			*end = 1;
		}
	} else {
		max = self->bytes_in_buffer;
	}

	/* maximum number of bytes we are reading */
	len = max < bytes-1 ? max : bytes-1;

	/* if we read any data... */
	if (len > 0) {

		/* copy the data */
		memcpy(buf, self->buf_begin, len);
		buf[len] = 0;

		if (bound && len > 0 && buf[len-1] == '\r') {
			buf[--len] = 0;
		}

		/* update the buffer */
		self->bytes_in_buffer -= (int)len;
		self->buf_begin += len;
	}

	return (int)len;
}