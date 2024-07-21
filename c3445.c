efind(name)
char *name;
{
	static char efbuf[100];
	my_regex_t re;

	sprintf(efbuf, "REG_%s", name);
	assert(strlen(efbuf) < sizeof(efbuf));
	re.re_endp = efbuf;
	(void) my_regerror(REG_ATOI, &re, efbuf, sizeof(efbuf));
	return(atoi(efbuf));
}