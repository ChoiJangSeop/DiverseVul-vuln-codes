static void decode_data(struct sixpack *sp, unsigned char inbyte)
{
	unsigned char *buf;

	if (sp->rx_count != 3) {
		sp->raw_buf[sp->rx_count++] = inbyte;

		return;
	}

	buf = sp->raw_buf;
	sp->cooked_buf[sp->rx_count_cooked++] =
		buf[0] | ((buf[1] << 2) & 0xc0);
	sp->cooked_buf[sp->rx_count_cooked++] =
		(buf[1] & 0x0f) | ((buf[2] << 2) & 0xf0);
	sp->cooked_buf[sp->rx_count_cooked++] =
		(buf[2] & 0x03) | (inbyte << 2);
	sp->rx_count = 0;
}