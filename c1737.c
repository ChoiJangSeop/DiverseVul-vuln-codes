CheckRelationOwnership(RangeVar *rel, bool noCatalogs)
{
	Oid			relOid;
	HeapTuple	tuple;

	/*
	 * XXX: This is unsafe in the presence of concurrent DDL, since it is
	 * called before acquiring any lock on the target relation.  However,
	 * locking the target relation (especially using something like
	 * AccessExclusiveLock) before verifying that the user has permissions is
	 * not appealing either.
	 */
	relOid = RangeVarGetRelid(rel, NoLock, false);

	tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(relOid));
	if (!HeapTupleIsValid(tuple))		/* should not happen */
		elog(ERROR, "cache lookup failed for relation %u", relOid);

	if (!pg_class_ownercheck(relOid, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_CLASS,
					   rel->relname);

	if (noCatalogs)
	{
		if (!allowSystemTableMods &&
			IsSystemClass(relOid, (Form_pg_class) GETSTRUCT(tuple)))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied: \"%s\" is a system catalog",
							rel->relname)));
	}

	ReleaseSysCache(tuple);
}