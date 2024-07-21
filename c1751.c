PGTYPESinterval_from_asc(char *str, char **endptr)
{
	interval   *result = NULL;
	fsec_t		fsec;
	struct tm	tt,
			   *tm = &tt;
	int			dtype;
	int			nf;
	char	   *field[MAXDATEFIELDS];
	int			ftype[MAXDATEFIELDS];
	char		lowstr[MAXDATELEN + MAXDATEFIELDS];
	char	   *realptr;
	char	  **ptr = (endptr != NULL) ? endptr : &realptr;

	tm->tm_year = 0;
	tm->tm_mon = 0;
	tm->tm_mday = 0;
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	fsec = 0;

	if (strlen(str) >= sizeof(lowstr))
	{
		errno = PGTYPES_INTVL_BAD_INTERVAL;
		return NULL;
	}

	if (ParseDateTime(str, lowstr, field, ftype, &nf, ptr) != 0 ||
		(DecodeInterval(field, ftype, nf, &dtype, tm, &fsec) != 0 &&
		 DecodeISO8601Interval(str, &dtype, tm, &fsec) != 0))
	{
		errno = PGTYPES_INTVL_BAD_INTERVAL;
		return NULL;
	}

	result = (interval *) pgtypes_alloc(sizeof(interval));
	if (!result)
		return NULL;

	if (dtype != DTK_DELTA)
	{
		errno = PGTYPES_INTVL_BAD_INTERVAL;
		free(result);
		return NULL;
	}

	if (tm2interval(tm, fsec, result) != 0)
	{
		errno = PGTYPES_INTVL_BAD_INTERVAL;
		free(result);
		return NULL;
	}

	errno = 0;
	return result;
}