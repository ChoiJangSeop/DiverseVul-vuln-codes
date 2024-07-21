static char *multipart_buffer_read_body(multipart_buffer *self, size_t *len)
{
	char buf[FILLUNIT], *out=NULL;
	int total_bytes=0, read_bytes=0;

	while((read_bytes = multipart_buffer_read(self, buf, sizeof(buf), NULL))) {
		out = erealloc(out, total_bytes + read_bytes + 1);
		memcpy(out + total_bytes, buf, read_bytes);
		total_bytes += read_bytes;
	}

	if (out) {
		out[total_bytes] = '\0';
	}
	*len = total_bytes;

	return out;
}