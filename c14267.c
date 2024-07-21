static uint get_alen(char *arg, int default_len)
{
	int	j;
	int	alen;

	alen = default_len;
	for (j = 0; j < 8; j++) {
		if (arg[j] == '.') {
			alen = arg[j+1] - '0';
			break;
		} else if (arg[j] == '\0')
			break;
	}
	return alen;
}