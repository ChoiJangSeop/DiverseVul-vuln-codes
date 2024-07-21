check_enable_rls(Oid relid, Oid checkAsUser)
{
	HeapTuple		tuple;
	Form_pg_class	classform;
	bool			relrowsecurity;
	Oid				user_id = checkAsUser ? checkAsUser : GetUserId();

	tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(tuple))
		return RLS_NONE;

	classform = (Form_pg_class) GETSTRUCT(tuple);

	relrowsecurity = classform->relrowsecurity;

	ReleaseSysCache(tuple);

	/* Nothing to do if the relation does not have RLS */
	if (!relrowsecurity)
		return RLS_NONE;

	/*
	 * Check permissions
	 *
	 * If the relation has row level security enabled and the row_security GUC
	 * is off, then check if the user has rights to bypass RLS for this
	 * relation.  Table owners can always bypass, as can any role with the
	 * BYPASSRLS capability.
	 *
	 * If the role is the table owner, then we bypass RLS unless row_security
	 * is set to 'force'.  Note that superuser is always considered an owner.
	 *
	 * Return RLS_NONE_ENV to indicate that this decision depends on the
	 * environment (in this case, what the current values of user_id and
	 * row_security are).
	 */
	if (row_security != ROW_SECURITY_FORCE
		&& (pg_class_ownercheck(relid, user_id)))
		return RLS_NONE_ENV;

	/*
	 * If the row_security GUC is 'off' then check if the user has permission
	 * to bypass it.  Note that we have already handled the case where the user
	 * is the table owner above.
	 *
	 * Note that row_security is always considered 'on' when querying
	 * through a view or other cases where checkAsUser is true, so skip this
	 * if checkAsUser is in use.
	 */
	if (!checkAsUser && row_security == ROW_SECURITY_OFF)
	{
		if (has_bypassrls_privilege(user_id))
			/* OK to bypass */
			return RLS_NONE_ENV;
		else
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("insufficient privilege to bypass row security.")));
	}

	/* RLS should be fully enabled for this relation. */
	return RLS_ENABLED;
}