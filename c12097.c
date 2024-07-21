char *strdup(const char *s1)
{
	char *s2 = 0;
	if (s1) {
		s2 = malloc(strlen(s1) + 1);
		strcpy(s2, s1);
	}
	return s2;
}