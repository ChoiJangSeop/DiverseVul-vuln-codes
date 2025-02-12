CollationCreate(const char *collname, Oid collnamespace,
				Oid collowner,
				char collprovider,
				bool collisdeterministic,
				int32 collencoding,
				const char *collcollate, const char *collctype,
				const char *colliculocale,
				const char *collversion,
				bool if_not_exists,
				bool quiet)
{
	Relation	rel;
	TupleDesc	tupDesc;
	HeapTuple	tup;
	Datum		values[Natts_pg_collation];
	bool		nulls[Natts_pg_collation];
	NameData	name_name;
	Oid			oid;
	ObjectAddress myself,
				referenced;

	AssertArg(collname);
	AssertArg(collnamespace);
	AssertArg(collowner);
	AssertArg((collcollate && collctype) || colliculocale);

	/*
	 * Make sure there is no existing collation of same name & encoding.
	 *
	 * This would be caught by the unique index anyway; we're just giving a
	 * friendlier error message.  The unique index provides a backstop against
	 * race conditions.
	 */
	if (SearchSysCacheExists3(COLLNAMEENCNSP,
							  PointerGetDatum(collname),
							  Int32GetDatum(collencoding),
							  ObjectIdGetDatum(collnamespace)))
	{
		if (quiet)
			return InvalidOid;
		else if (if_not_exists)
		{
			ereport(NOTICE,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 collencoding == -1
					 ? errmsg("collation \"%s\" already exists, skipping",
							  collname)
					 : errmsg("collation \"%s\" for encoding \"%s\" already exists, skipping",
							  collname, pg_encoding_to_char(collencoding))));
			return InvalidOid;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 collencoding == -1
					 ? errmsg("collation \"%s\" already exists",
							  collname)
					 : errmsg("collation \"%s\" for encoding \"%s\" already exists",
							  collname, pg_encoding_to_char(collencoding))));
	}

	/* open pg_collation; see below about the lock level */
	rel = table_open(CollationRelationId, ShareRowExclusiveLock);

	/*
	 * Also forbid a specific-encoding collation shadowing an any-encoding
	 * collation, or an any-encoding collation being shadowed (see
	 * get_collation_name()).  This test is not backed up by the unique index,
	 * so we take a ShareRowExclusiveLock earlier, to protect against
	 * concurrent changes fooling this check.
	 */
	if ((collencoding == -1 &&
		 SearchSysCacheExists3(COLLNAMEENCNSP,
							   PointerGetDatum(collname),
							   Int32GetDatum(GetDatabaseEncoding()),
							   ObjectIdGetDatum(collnamespace))) ||
		(collencoding != -1 &&
		 SearchSysCacheExists3(COLLNAMEENCNSP,
							   PointerGetDatum(collname),
							   Int32GetDatum(-1),
							   ObjectIdGetDatum(collnamespace))))
	{
		if (quiet)
		{
			table_close(rel, NoLock);
			return InvalidOid;
		}
		else if (if_not_exists)
		{
			table_close(rel, NoLock);
			ereport(NOTICE,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("collation \"%s\" already exists, skipping",
							collname)));
			return InvalidOid;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("collation \"%s\" already exists",
							collname)));
	}

	tupDesc = RelationGetDescr(rel);

	/* form a tuple */
	memset(nulls, 0, sizeof(nulls));

	namestrcpy(&name_name, collname);
	oid = GetNewOidWithIndex(rel, CollationOidIndexId,
							 Anum_pg_collation_oid);
	values[Anum_pg_collation_oid - 1] = ObjectIdGetDatum(oid);
	values[Anum_pg_collation_collname - 1] = NameGetDatum(&name_name);
	values[Anum_pg_collation_collnamespace - 1] = ObjectIdGetDatum(collnamespace);
	values[Anum_pg_collation_collowner - 1] = ObjectIdGetDatum(collowner);
	values[Anum_pg_collation_collprovider - 1] = CharGetDatum(collprovider);
	values[Anum_pg_collation_collisdeterministic - 1] = BoolGetDatum(collisdeterministic);
	values[Anum_pg_collation_collencoding - 1] = Int32GetDatum(collencoding);
	if (collcollate)
		values[Anum_pg_collation_collcollate - 1] = CStringGetTextDatum(collcollate);
	else
		nulls[Anum_pg_collation_collcollate - 1] = true;
	if (collctype)
		values[Anum_pg_collation_collctype - 1] = CStringGetTextDatum(collctype);
	else
		nulls[Anum_pg_collation_collctype - 1] = true;
	if (colliculocale)
		values[Anum_pg_collation_colliculocale - 1] = CStringGetTextDatum(colliculocale);
	else
		nulls[Anum_pg_collation_colliculocale - 1] = true;
	if (collversion)
		values[Anum_pg_collation_collversion - 1] = CStringGetTextDatum(collversion);
	else
		nulls[Anum_pg_collation_collversion - 1] = true;

	tup = heap_form_tuple(tupDesc, values, nulls);

	/* insert a new tuple */
	CatalogTupleInsert(rel, tup);
	Assert(OidIsValid(oid));

	/* set up dependencies for the new collation */
	myself.classId = CollationRelationId;
	myself.objectId = oid;
	myself.objectSubId = 0;

	/* create dependency on namespace */
	referenced.classId = NamespaceRelationId;
	referenced.objectId = collnamespace;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	/* create dependency on owner */
	recordDependencyOnOwner(CollationRelationId, oid, collowner);

	/* dependency on extension */
	recordDependencyOnCurrentExtension(&myself, false);

	/* Post creation hook for new collation */
	InvokeObjectPostCreateHook(CollationRelationId, oid, 0);

	heap_freetuple(tup);
	table_close(rel, NoLock);

	return oid;
}