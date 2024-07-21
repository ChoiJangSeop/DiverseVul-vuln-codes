char *jas_strdup(const char *s)
{
	int n;
	char *p;
	n = strlen(s) + 1;
	if (!(p = jas_malloc(n * sizeof(char)))) {
		return 0;
	}
	strcpy(p, s);
	return p;
}