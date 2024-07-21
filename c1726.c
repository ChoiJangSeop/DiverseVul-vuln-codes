fmgr_internal_validator(PG_FUNCTION_ARGS)
{
	Oid			funcoid = PG_GETARG_OID(0);
	HeapTuple	tuple;
	bool		isnull;
	Datum		tmp;
	char	   *prosrc;

	/*
	 * We do not honor check_function_bodies since it's unlikely the function
	 * name will be found later if it isn't there now.
	 */

	tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(funcoid));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for function %u", funcoid);

	tmp = SysCacheGetAttr(PROCOID, tuple, Anum_pg_proc_prosrc, &isnull);
	if (isnull)
		elog(ERROR, "null prosrc");
	prosrc = TextDatumGetCString(tmp);

	if (fmgr_internal_function(prosrc) == InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("there is no built-in function named \"%s\"",
						prosrc)));

	ReleaseSysCache(tuple);

	PG_RETURN_VOID();
}