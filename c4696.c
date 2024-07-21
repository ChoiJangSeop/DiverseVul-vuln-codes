stream_read(cdk_stream_t s, void *buf, size_t buflen, size_t * r_nread)
{
	int res = cdk_stream_read(s, buf, buflen);
	if (res > 0) {
		*r_nread = res;
		return 0;
	} else {
		return (cdk_stream_eof(s) ? EOF : _cdk_stream_get_errno(s));
	}
}