static u32 read_32(cdk_stream_t s)
{
	byte buf[4];
	size_t nread;

	assert(s != NULL);

	stream_read(s, buf, 4, &nread);
	if (nread != 4)
		return (u32) - 1;
	return buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
}