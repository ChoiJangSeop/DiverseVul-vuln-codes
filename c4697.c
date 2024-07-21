static u16 read_16(cdk_stream_t s)
{
	byte buf[2];
	size_t nread;

	assert(s != NULL);

	stream_read(s, buf, 2, &nread);
	if (nread != 2)
		return (u16) - 1;
	return buf[0] << 8 | buf[1];
}