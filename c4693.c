read_old_length(cdk_stream_t inp, int ctb, size_t * r_len, size_t * r_size)
{
	int llen = ctb & 0x03;

	if (llen == 0) {
		*r_len = cdk_stream_getc(inp);
		(*r_size)++;
	} else if (llen == 1) {
		*r_len = read_16(inp);
		(*r_size) += 2;
	} else if (llen == 2) {
		*r_len = read_32(inp);
		(*r_size) += 4;
	} else {
		*r_len = 0;
		*r_size = 0;
	}
}