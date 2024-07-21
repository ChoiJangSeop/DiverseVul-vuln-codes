is_admin_of_role(Oid member, Oid role)
{
	bool		result = false;
	List	   *roles_list;
	ListCell   *l;

	/* Fast path for simple case */
	if (member == role)
		return true;

	/* Superusers have every privilege, so are part of every role */
	if (superuser_arg(member))
		return true;

	/*
	 * Find all the roles that member is a member of, including multi-level
	 * recursion.  We build a list in the same way that is_member_of_role does
	 * to track visited and unvisited roles.
	 */
	roles_list = list_make1_oid(member);

	foreach(l, roles_list)
	{
		Oid			memberid = lfirst_oid(l);
		CatCList   *memlist;
		int			i;

		/* Find roles that memberid is directly a member of */
		memlist = SearchSysCacheList1(AUTHMEMMEMROLE,
									  ObjectIdGetDatum(memberid));
		for (i = 0; i < memlist->n_members; i++)
		{
			HeapTuple	tup = &memlist->members[i]->tuple;
			Oid			otherid = ((Form_pg_auth_members) GETSTRUCT(tup))->roleid;

			if (otherid == role &&
				((Form_pg_auth_members) GETSTRUCT(tup))->admin_option)
			{
				/* Found what we came for, so can stop searching */
				result = true;
				break;
			}

			roles_list = list_append_unique_oid(roles_list, otherid);
		}
		ReleaseSysCacheList(memlist);
		if (result)
			break;
	}

	list_free(roles_list);

	return result;
}