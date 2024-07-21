cdf_dump(void *v, size_t len)
{
	size_t i, j;
	unsigned char *p = v;
	char abuf[16];
	(void)fprintf(stderr, "%.4x: ", 0);
	for (i = 0, j = 0; i < len; i++, p++) {
		(void)fprintf(stderr, "%.2x ", *p);
		abuf[j++] = isprint(*p) ? *p : '.';
		if (j == 16) {
			j = 0;
			abuf[15] = '\0';
			(void)fprintf(stderr, "%s\n%.4x: ", abuf, i + 1);
		}
	}
	(void)fprintf(stderr, "\n");
}