static void hexprint(FILE *out, const unsigned char *buf, int buflen)
	{
	int i;
	fprintf(out, "\t");
	for (i = 0; i < buflen; i++)
		fprintf(out, "%02X", buf[i]);
	fprintf(out, "\n");
	}