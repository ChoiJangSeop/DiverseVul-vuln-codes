PGTYPEStimestamp_from_asc(char *str, char **endptr)
{
	timestamp	result;

#ifdef HAVE_INT64_TIMESTAMP
	int64		noresult = 0;
#else
	double		noresult = 0.0;
#endif
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

	if (strlen(str) >= sizeof(lowstr))
	{
		errno = PGTYPES_TS_BAD_TIMESTAMP;
		return (noresult);
	}

	if (ParseDateTime(str, lowstr, field, ftype, &nf, ptr) != 0 ||
		DecodeDateTime(field, ftype, nf, &dtype, tm, &fsec, 0) != 0)
	{
		errno = PGTYPES_TS_BAD_TIMESTAMP;
		return (noresult);
	}

	switch (dtype)
	{
		case DTK_DATE:
			if (tm2timestamp(tm, fsec, NULL, &result) != 0)
			{
				errno = PGTYPES_TS_BAD_TIMESTAMP;
				return (noresult);
			}
			break;

		case DTK_EPOCH:
			result = SetEpochTimestamp();
			break;

		case DTK_LATE:
			TIMESTAMP_NOEND(result);
			break;

		case DTK_EARLY:
			TIMESTAMP_NOBEGIN(result);
			break;

		case DTK_INVALID:
			errno = PGTYPES_TS_BAD_TIMESTAMP;
			return (noresult);

		default:
			errno = PGTYPES_TS_BAD_TIMESTAMP;
			return (noresult);
	}

	/* AdjustTimestampForTypmod(&result, typmod); */

	/*
	 * Since it's difficult to test for noresult, make sure errno is 0 if no
	 * error occurred.
	 */
	errno = 0;
	return result;
}