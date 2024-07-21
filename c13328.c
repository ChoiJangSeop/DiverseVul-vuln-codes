CreateUserMapping(CreateUserMappingStmt *stmt)
{
	Relation	rel;
	Datum		useoptions;
	Datum		values[Natts_pg_user_mapping];
	bool		nulls[Natts_pg_user_mapping];
	HeapTuple	tuple;
	Oid			useId;
	Oid			umId;
	ObjectAddress myself;
	ObjectAddress referenced;
	ForeignServer *srv;
	ForeignDataWrapper *fdw;
	RoleSpec   *role = (RoleSpec *) stmt->user;

	rel = table_open(UserMappingRelationId, RowExclusiveLock);

	if (role->roletype == ROLESPEC_PUBLIC)
		useId = ACL_ID_PUBLIC;
	else
		useId = get_rolespec_oid(stmt->user, false);

	/* Check that the server exists. */
	srv = GetForeignServerByName(stmt->servername, false);

	user_mapping_ddl_aclcheck(useId, srv->serverid, stmt->servername);

	/*
	 * Check that the user mapping is unique within server.
	 */
	umId = GetSysCacheOid2(USERMAPPINGUSERSERVER, Anum_pg_user_mapping_oid,
						   ObjectIdGetDatum(useId),
						   ObjectIdGetDatum(srv->serverid));

	if (OidIsValid(umId))
	{
		if (stmt->if_not_exists)
		{
			ereport(NOTICE,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("user mapping for \"%s\" already exists for server \"%s\", skipping",
							MappingUserName(useId),
							stmt->servername)));

			table_close(rel, RowExclusiveLock);
			return InvalidObjectAddress;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("user mapping for \"%s\" already exists for server \"%s\"",
							MappingUserName(useId),
							stmt->servername)));
	}

	fdw = GetForeignDataWrapper(srv->fdwid);

	/*
	 * Insert tuple into pg_user_mapping.
	 */
	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	umId = GetNewOidWithIndex(rel, UserMappingOidIndexId,
							  Anum_pg_user_mapping_oid);
	values[Anum_pg_user_mapping_oid - 1] = ObjectIdGetDatum(umId);
	values[Anum_pg_user_mapping_umuser - 1] = ObjectIdGetDatum(useId);
	values[Anum_pg_user_mapping_umserver - 1] = ObjectIdGetDatum(srv->serverid);

	/* Add user options */
	useoptions = transformGenericOptions(UserMappingRelationId,
										 PointerGetDatum(NULL),
										 stmt->options,
										 fdw->fdwvalidator);

	if (PointerIsValid(DatumGetPointer(useoptions)))
		values[Anum_pg_user_mapping_umoptions - 1] = useoptions;
	else
		nulls[Anum_pg_user_mapping_umoptions - 1] = true;

	tuple = heap_form_tuple(rel->rd_att, values, nulls);

	CatalogTupleInsert(rel, tuple);

	heap_freetuple(tuple);

	/* Add dependency on the server */
	myself.classId = UserMappingRelationId;
	myself.objectId = umId;
	myself.objectSubId = 0;

	referenced.classId = ForeignServerRelationId;
	referenced.objectId = srv->serverid;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	if (OidIsValid(useId))
	{
		/* Record the mapped user dependency */
		recordDependencyOnOwner(UserMappingRelationId, umId, useId);
	}

	/*
	 * Perhaps someday there should be a recordDependencyOnCurrentExtension
	 * call here; but since roles aren't members of extensions, it seems like
	 * user mappings shouldn't be either.  Note that the grammar and pg_dump
	 * would need to be extended too if we change this.
	 */

	/* Post creation hook for new user mapping */
	InvokeObjectPostCreateHook(UserMappingRelationId, umId, 0);

	table_close(rel, RowExclusiveLock);

	return myself;
}