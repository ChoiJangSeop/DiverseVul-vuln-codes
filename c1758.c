path_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	PATH	   *path;
	int			isopen;
	char	   *s;
	int			npts;
	int			size;
	int			depth = 0;

	if ((npts = pair_count(str, ',')) <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type path: \"%s\"", str)));

	s = str;
	while (isspace((unsigned char) *s))
		s++;

	/* skip single leading paren */
	if ((*s == LDELIM) && (strrchr(s, LDELIM) == s))
	{
		s++;
		depth++;
	}

	size = offsetof(PATH, p[0]) +sizeof(path->p[0]) * npts;
	path = (PATH *) palloc(size);

	SET_VARSIZE(path, size);
	path->npts = npts;

	if ((!path_decode(TRUE, npts, s, &isopen, &s, &(path->p[0])))
	&& (!((depth == 0) && (*s == '\0'))) && !((depth >= 1) && (*s == RDELIM)))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type path: \"%s\"", str)));

	path->closed = (!isopen);
	/* prevent instability in unused pad bytes */
	path->dummy = 0;

	PG_RETURN_PATH_P(path);
}