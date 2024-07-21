CreateForeignServer(CreateForeignServerStmt *stmt)
{
	Relation	rel;
	Datum		srvoptions;
	Datum		values[Natts_pg_foreign_server];
	bool		nulls[Natts_pg_foreign_server];
	HeapTuple	tuple;
	Oid			srvId;
	Oid			ownerId;
	AclResult	aclresult;
	ObjectAddress myself;
	ObjectAddress referenced;
	ForeignDataWrapper *fdw;

	rel = heap_open(ForeignServerRelationId, RowExclusiveLock);

	/* For now the owner cannot be specified on create. Use effective user ID. */
	ownerId = GetUserId();

	/*
	 * Check that there is no other foreign server by this name. Do nothing if
	 * IF NOT EXISTS was enforced.
	 */
	if (GetForeignServerByName(stmt->servername, true) != NULL)
	{
		if (stmt->if_not_exists)
		{
			ereport(NOTICE,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("server \"%s\" already exists, skipping",
							stmt->servername)));
			heap_close(rel, RowExclusiveLock);
			return InvalidObjectAddress;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("server \"%s\" already exists",
							stmt->servername)));
	}

	/*
	 * Check that the FDW exists and that we have USAGE on it. Also get the
	 * actual FDW for option validation etc.
	 */
	fdw = GetForeignDataWrapperByName(stmt->fdwname, false);

	aclresult = pg_foreign_data_wrapper_aclcheck(fdw->fdwid, ownerId, ACL_USAGE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, OBJECT_FDW, fdw->fdwname);

	/*
	 * Insert tuple into pg_foreign_server.
	 */
	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	values[Anum_pg_foreign_server_srvname - 1] =
		DirectFunctionCall1(namein, CStringGetDatum(stmt->servername));
	values[Anum_pg_foreign_server_srvowner - 1] = ObjectIdGetDatum(ownerId);
	values[Anum_pg_foreign_server_srvfdw - 1] = ObjectIdGetDatum(fdw->fdwid);

	/* Add server type if supplied */
	if (stmt->servertype)
		values[Anum_pg_foreign_server_srvtype - 1] =
			CStringGetTextDatum(stmt->servertype);
	else
		nulls[Anum_pg_foreign_server_srvtype - 1] = true;

	/* Add server version if supplied */
	if (stmt->version)
		values[Anum_pg_foreign_server_srvversion - 1] =
			CStringGetTextDatum(stmt->version);
	else
		nulls[Anum_pg_foreign_server_srvversion - 1] = true;

	/* Start with a blank acl */
	nulls[Anum_pg_foreign_server_srvacl - 1] = true;

	/* Add server options */
	srvoptions = transformGenericOptions(ForeignServerRelationId,
										 PointerGetDatum(NULL),
										 stmt->options,
										 fdw->fdwvalidator);

	if (PointerIsValid(DatumGetPointer(srvoptions)))
		values[Anum_pg_foreign_server_srvoptions - 1] = srvoptions;
	else
		nulls[Anum_pg_foreign_server_srvoptions - 1] = true;

	tuple = heap_form_tuple(rel->rd_att, values, nulls);

	srvId = CatalogTupleInsert(rel, tuple);

	heap_freetuple(tuple);

	/* record dependencies */
	myself.classId = ForeignServerRelationId;
	myself.objectId = srvId;
	myself.objectSubId = 0;

	referenced.classId = ForeignDataWrapperRelationId;
	referenced.objectId = fdw->fdwid;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	recordDependencyOnOwner(ForeignServerRelationId, srvId, ownerId);

	/* dependency on extension */
	recordDependencyOnCurrentExtension(&myself, false);

	/* Post creation hook for new foreign server */
	InvokeObjectPostCreateHook(ForeignServerRelationId, srvId, 0);

	heap_close(rel, RowExclusiveLock);

	return myself;
}