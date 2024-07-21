AddRoleMems(const char *rolename, Oid roleid,
			List *memberNames, List *memberIds,
			Oid grantorId, bool admin_opt)
{
	Relation	pg_authmem_rel;
	TupleDesc	pg_authmem_dsc;
	ListCell   *nameitem;
	ListCell   *iditem;

	Assert(list_length(memberNames) == list_length(memberIds));

	/* Skip permission check if nothing to do */
	if (!memberIds)
		return;

	/*
	 * Check permissions: must have createrole or admin option on the role to
	 * be changed.	To mess with a superuser role, you gotta be superuser.
	 */
	if (superuser_arg(roleid))
	{
		if (!superuser())
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("must be superuser to alter superusers")));
	}
	else
	{
		if (!have_createrole_privilege() &&
			!is_admin_of_role(grantorId, roleid))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("must have admin option on role \"%s\"",
							rolename)));
	}

	/* XXX not sure about this check */
	if (grantorId != GetUserId() && !superuser())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("must be superuser to set grantor")));

	pg_authmem_rel = heap_open(AuthMemRelationId, RowExclusiveLock);
	pg_authmem_dsc = RelationGetDescr(pg_authmem_rel);

	forboth(nameitem, memberNames, iditem, memberIds)
	{
		const char *membername = strVal(lfirst(nameitem));
		Oid			memberid = lfirst_oid(iditem);
		HeapTuple	authmem_tuple;
		HeapTuple	tuple;
		Datum		new_record[Natts_pg_auth_members];
		bool		new_record_nulls[Natts_pg_auth_members];
		bool		new_record_repl[Natts_pg_auth_members];

		/*
		 * Refuse creation of membership loops, including the trivial case
		 * where a role is made a member of itself.  We do this by checking to
		 * see if the target role is already a member of the proposed member
		 * role.  We have to ignore possible superuserness, however, else we
		 * could never grant membership in a superuser-privileged role.
		 */
		if (is_member_of_role_nosuper(roleid, memberid))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_GRANT_OPERATION),
					 (errmsg("role \"%s\" is a member of role \"%s\"",
							 rolename, membername))));

		/*
		 * Check if entry for this role/member already exists; if so, give
		 * warning unless we are adding admin option.
		 */
		authmem_tuple = SearchSysCache2(AUTHMEMROLEMEM,
										ObjectIdGetDatum(roleid),
										ObjectIdGetDatum(memberid));
		if (HeapTupleIsValid(authmem_tuple) &&
			(!admin_opt ||
			 ((Form_pg_auth_members) GETSTRUCT(authmem_tuple))->admin_option))
		{
			ereport(NOTICE,
					(errmsg("role \"%s\" is already a member of role \"%s\"",
							membername, rolename)));
			ReleaseSysCache(authmem_tuple);
			continue;
		}

		/* Build a tuple to insert or update */
		MemSet(new_record, 0, sizeof(new_record));
		MemSet(new_record_nulls, false, sizeof(new_record_nulls));
		MemSet(new_record_repl, false, sizeof(new_record_repl));

		new_record[Anum_pg_auth_members_roleid - 1] = ObjectIdGetDatum(roleid);
		new_record[Anum_pg_auth_members_member - 1] = ObjectIdGetDatum(memberid);
		new_record[Anum_pg_auth_members_grantor - 1] = ObjectIdGetDatum(grantorId);
		new_record[Anum_pg_auth_members_admin_option - 1] = BoolGetDatum(admin_opt);

		if (HeapTupleIsValid(authmem_tuple))
		{
			new_record_repl[Anum_pg_auth_members_grantor - 1] = true;
			new_record_repl[Anum_pg_auth_members_admin_option - 1] = true;
			tuple = heap_modify_tuple(authmem_tuple, pg_authmem_dsc,
									  new_record,
									  new_record_nulls, new_record_repl);
			simple_heap_update(pg_authmem_rel, &tuple->t_self, tuple);
			CatalogUpdateIndexes(pg_authmem_rel, tuple);
			ReleaseSysCache(authmem_tuple);
		}
		else
		{
			tuple = heap_form_tuple(pg_authmem_dsc,
									new_record, new_record_nulls);
			simple_heap_insert(pg_authmem_rel, tuple);
			CatalogUpdateIndexes(pg_authmem_rel, tuple);
		}

		/* CCI after each change, in case there are duplicates in list */
		CommandCounterIncrement();
	}

	/*
	 * Close pg_authmem, but keep lock till commit.
	 */
	heap_close(pg_authmem_rel, NoLock);
}