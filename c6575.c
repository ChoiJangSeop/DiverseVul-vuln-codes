static int chap_string_to_hex(unsigned char *dst, unsigned char *src, int len)
{
	int j = DIV_ROUND_UP(len, 2), rc;

	rc = hex2bin(dst, src, j);
	if (rc < 0)
		pr_debug("CHAP string contains non hex digit symbols\n");

	dst[j] = '\0';
	return j;
}