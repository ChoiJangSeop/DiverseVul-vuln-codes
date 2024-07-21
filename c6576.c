static void chap_binaryhex_to_asciihex(char *dst, char *src, int src_len)
{
	int i;

	for (i = 0; i < src_len; i++) {
		sprintf(&dst[i*2], "%02x", (int) src[i] & 0xff);
	}
}