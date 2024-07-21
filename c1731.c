DefineIndex(IndexStmt *stmt,
			Oid indexRelationId,
			bool is_alter_table,
			bool check_rights,
			bool skip_build,
			bool quiet)
{
	char	   *indexRelationName;
	char	   *accessMethodName;
	Oid		   *typeObjectId;
	Oid		   *collationObjectId;
	Oid		   *classObjectId;
	Oid			accessMethodId;
	Oid			relationId;
	Oid			namespaceId;
	Oid			tablespaceId;
	List	   *indexColNames;
	Relation	rel;
	Relation	indexRelation;
	HeapTuple	tuple;
	Form_pg_am	accessMethodForm;
	bool		amcanorder;
	RegProcedure amoptions;
	Datum		reloptions;
	int16	   *coloptions;
	IndexInfo  *indexInfo;
	int			numberOfAttributes;
	TransactionId limitXmin;
	VirtualTransactionId *old_snapshots;
	int			n_old_snapshots;
	LockRelId	heaprelid;
	LOCKTAG		heaplocktag;
	Snapshot	snapshot;
	int			i;

	/*
	 * count attributes in index
	 */
	numberOfAttributes = list_length(stmt->indexParams);
	if (numberOfAttributes <= 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
				 errmsg("must specify at least one column")));
	if (numberOfAttributes > INDEX_MAX_KEYS)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_COLUMNS),
				 errmsg("cannot use more than %d columns in an index",
						INDEX_MAX_KEYS)));

	/*
	 * Open heap relation, acquire a suitable lock on it, remember its OID
	 *
	 * Only SELECT ... FOR UPDATE/SHARE are allowed while doing a standard
	 * index build; but for concurrent builds we allow INSERT/UPDATE/DELETE
	 * (but not VACUUM).
	 */
	rel = heap_openrv(stmt->relation,
				  (stmt->concurrent ? ShareUpdateExclusiveLock : ShareLock));

	relationId = RelationGetRelid(rel);
	namespaceId = RelationGetNamespace(rel);

	if (rel->rd_rel->relkind != RELKIND_RELATION &&
		rel->rd_rel->relkind != RELKIND_MATVIEW)
	{
		if (rel->rd_rel->relkind == RELKIND_FOREIGN_TABLE)

			/*
			 * Custom error message for FOREIGN TABLE since the term is close
			 * to a regular table and can confuse the user.
			 */
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot create index on foreign table \"%s\"",
							RelationGetRelationName(rel))));
		else
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("\"%s\" is not a table or materialized view",
							RelationGetRelationName(rel))));
	}

	/*
	 * Don't try to CREATE INDEX on temp tables of other backends.
	 */
	if (RELATION_IS_OTHER_TEMP(rel))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot create indexes on temporary tables of other sessions")));

	/*
	 * Verify we (still) have CREATE rights in the rel's namespace.
	 * (Presumably we did when the rel was created, but maybe not anymore.)
	 * Skip check if caller doesn't want it.  Also skip check if
	 * bootstrapping, since permissions machinery may not be working yet.
	 */
	if (check_rights && !IsBootstrapProcessingMode())
	{
		AclResult	aclresult;

		aclresult = pg_namespace_aclcheck(namespaceId, GetUserId(),
										  ACL_CREATE);
		if (aclresult != ACLCHECK_OK)
			aclcheck_error(aclresult, ACL_KIND_NAMESPACE,
						   get_namespace_name(namespaceId));
	}

	/*
	 * Select tablespace to use.  If not specified, use default tablespace
	 * (which may in turn default to database's default).
	 */
	if (stmt->tableSpace)
	{
		tablespaceId = get_tablespace_oid(stmt->tableSpace, false);
	}
	else
	{
		tablespaceId = GetDefaultTablespace(rel->rd_rel->relpersistence);
		/* note InvalidOid is OK in this case */
	}

	/* Check permissions except when using database's default */
	if (OidIsValid(tablespaceId) && tablespaceId != MyDatabaseTableSpace)
	{
		AclResult	aclresult;

		aclresult = pg_tablespace_aclcheck(tablespaceId, GetUserId(),
										   ACL_CREATE);
		if (aclresult != ACLCHECK_OK)
			aclcheck_error(aclresult, ACL_KIND_TABLESPACE,
						   get_tablespace_name(tablespaceId));
	}

	/*
	 * Force shared indexes into the pg_global tablespace.	This is a bit of a
	 * hack but seems simpler than marking them in the BKI commands.  On the
	 * other hand, if it's not shared, don't allow it to be placed there.
	 */
	if (rel->rd_rel->relisshared)
		tablespaceId = GLOBALTABLESPACE_OID;
	else if (tablespaceId == GLOBALTABLESPACE_OID)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("only shared relations can be placed in pg_global tablespace")));

	/*
	 * Choose the index column names.
	 */
	indexColNames = ChooseIndexColumnNames(stmt->indexParams);

	/*
	 * Select name for index if caller didn't specify
	 */
	indexRelationName = stmt->idxname;
	if (indexRelationName == NULL)
		indexRelationName = ChooseIndexName(RelationGetRelationName(rel),
											namespaceId,
											indexColNames,
											stmt->excludeOpNames,
											stmt->primary,
											stmt->isconstraint);

	/*
	 * look up the access method, verify it can handle the requested features
	 */
	accessMethodName = stmt->accessMethod;
	tuple = SearchSysCache1(AMNAME, PointerGetDatum(accessMethodName));
	if (!HeapTupleIsValid(tuple))
	{
		/*
		 * Hack to provide more-or-less-transparent updating of old RTREE
		 * indexes to GiST: if RTREE is requested and not found, use GIST.
		 */
		if (strcmp(accessMethodName, "rtree") == 0)
		{
			ereport(NOTICE,
					(errmsg("substituting access method \"gist\" for obsolete method \"rtree\"")));
			accessMethodName = "gist";
			tuple = SearchSysCache1(AMNAME, PointerGetDatum(accessMethodName));
		}

		if (!HeapTupleIsValid(tuple))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("access method \"%s\" does not exist",
							accessMethodName)));
	}
	accessMethodId = HeapTupleGetOid(tuple);
	accessMethodForm = (Form_pg_am) GETSTRUCT(tuple);

	if (stmt->unique && !accessMethodForm->amcanunique)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			   errmsg("access method \"%s\" does not support unique indexes",
					  accessMethodName)));
	if (numberOfAttributes > 1 && !accessMethodForm->amcanmulticol)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		  errmsg("access method \"%s\" does not support multicolumn indexes",
				 accessMethodName)));
	if (stmt->excludeOpNames && !OidIsValid(accessMethodForm->amgettuple))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		errmsg("access method \"%s\" does not support exclusion constraints",
			   accessMethodName)));

	amcanorder = accessMethodForm->amcanorder;
	amoptions = accessMethodForm->amoptions;

	ReleaseSysCache(tuple);

	/*
	 * Validate predicate, if given
	 */
	if (stmt->whereClause)
		CheckPredicate((Expr *) stmt->whereClause);

	/*
	 * Parse AM-specific options, convert to text array form, validate.
	 */
	reloptions = transformRelOptions((Datum) 0, stmt->options,
									 NULL, NULL, false, false);

	(void) index_reloptions(amoptions, reloptions, true);

	/*
	 * Prepare arguments for index_create, primarily an IndexInfo structure.
	 * Note that ii_Predicate must be in implicit-AND format.
	 */
	indexInfo = makeNode(IndexInfo);
	indexInfo->ii_NumIndexAttrs = numberOfAttributes;
	indexInfo->ii_Expressions = NIL;	/* for now */
	indexInfo->ii_ExpressionsState = NIL;
	indexInfo->ii_Predicate = make_ands_implicit((Expr *) stmt->whereClause);
	indexInfo->ii_PredicateState = NIL;
	indexInfo->ii_ExclusionOps = NULL;
	indexInfo->ii_ExclusionProcs = NULL;
	indexInfo->ii_ExclusionStrats = NULL;
	indexInfo->ii_Unique = stmt->unique;
	/* In a concurrent build, mark it not-ready-for-inserts */
	indexInfo->ii_ReadyForInserts = !stmt->concurrent;
	indexInfo->ii_Concurrent = stmt->concurrent;
	indexInfo->ii_BrokenHotChain = false;

	typeObjectId = (Oid *) palloc(numberOfAttributes * sizeof(Oid));
	collationObjectId = (Oid *) palloc(numberOfAttributes * sizeof(Oid));
	classObjectId = (Oid *) palloc(numberOfAttributes * sizeof(Oid));
	coloptions = (int16 *) palloc(numberOfAttributes * sizeof(int16));
	ComputeIndexAttrs(indexInfo,
					  typeObjectId, collationObjectId, classObjectId,
					  coloptions, stmt->indexParams,
					  stmt->excludeOpNames, relationId,
					  accessMethodName, accessMethodId,
					  amcanorder, stmt->isconstraint);

	/*
	 * Extra checks when creating a PRIMARY KEY index.
	 */
	if (stmt->primary)
		index_check_primary_key(rel, indexInfo, is_alter_table);

	/*
	 * Report index creation if appropriate (delay this till after most of the
	 * error checks)
	 */
	if (stmt->isconstraint && !quiet)
	{
		const char *constraint_type;

		if (stmt->primary)
			constraint_type = "PRIMARY KEY";
		else if (stmt->unique)
			constraint_type = "UNIQUE";
		else if (stmt->excludeOpNames != NIL)
			constraint_type = "EXCLUDE";
		else
		{
			elog(ERROR, "unknown constraint type");
			constraint_type = NULL;		/* keep compiler quiet */
		}

		ereport(DEBUG1,
		  (errmsg("%s %s will create implicit index \"%s\" for table \"%s\"",
				  is_alter_table ? "ALTER TABLE / ADD" : "CREATE TABLE /",
				  constraint_type,
				  indexRelationName, RelationGetRelationName(rel))));
	}

	/*
	 * A valid stmt->oldNode implies that we already have a built form of the
	 * index.  The caller should also decline any index build.
	 */
	Assert(!OidIsValid(stmt->oldNode) || (skip_build && !stmt->concurrent));

	/*
	 * Make the catalog entries for the index, including constraints. Then, if
	 * not skip_build || concurrent, actually build the index.
	 */
	indexRelationId =
		index_create(rel, indexRelationName, indexRelationId, stmt->oldNode,
					 indexInfo, indexColNames,
					 accessMethodId, tablespaceId,
					 collationObjectId, classObjectId,
					 coloptions, reloptions, stmt->primary,
					 stmt->isconstraint, stmt->deferrable, stmt->initdeferred,
					 allowSystemTableMods,
					 skip_build || stmt->concurrent,
					 stmt->concurrent, !check_rights);

	/* Add any requested comment */
	if (stmt->idxcomment != NULL)
		CreateComments(indexRelationId, RelationRelationId, 0,
					   stmt->idxcomment);

	if (!stmt->concurrent)
	{
		/* Close the heap and we're done, in the non-concurrent case */
		heap_close(rel, NoLock);
		return indexRelationId;
	}

	/* save lockrelid and locktag for below, then close rel */
	heaprelid = rel->rd_lockInfo.lockRelId;
	SET_LOCKTAG_RELATION(heaplocktag, heaprelid.dbId, heaprelid.relId);
	heap_close(rel, NoLock);

	/*
	 * For a concurrent build, it's important to make the catalog entries
	 * visible to other transactions before we start to build the index. That
	 * will prevent them from making incompatible HOT updates.	The new index
	 * will be marked not indisready and not indisvalid, so that no one else
	 * tries to either insert into it or use it for queries.
	 *
	 * We must commit our current transaction so that the index becomes
	 * visible; then start another.  Note that all the data structures we just
	 * built are lost in the commit.  The only data we keep past here are the
	 * relation IDs.
	 *
	 * Before committing, get a session-level lock on the table, to ensure
	 * that neither it nor the index can be dropped before we finish. This
	 * cannot block, even if someone else is waiting for access, because we
	 * already have the same lock within our transaction.
	 *
	 * Note: we don't currently bother with a session lock on the index,
	 * because there are no operations that could change its state while we
	 * hold lock on the parent table.  This might need to change later.
	 */
	LockRelationIdForSession(&heaprelid, ShareUpdateExclusiveLock);

	PopActiveSnapshot();
	CommitTransactionCommand();
	StartTransactionCommand();

	/*
	 * Phase 2 of concurrent index build (see comments for validate_index()
	 * for an overview of how this works)
	 *
	 * Now we must wait until no running transaction could have the table open
	 * with the old list of indexes.  Use ShareLock to consider running
	 * transactions that hold locks that permit writing to the table.  Note we
	 * do not need to worry about xacts that open the table for writing after
	 * this point; they will see the new index when they open it.
	 *
	 * Note: the reason we use actual lock acquisition here, rather than just
	 * checking the ProcArray and sleeping, is that deadlock is possible if
	 * one of the transactions in question is blocked trying to acquire an
	 * exclusive lock on our table.  The lock code will detect deadlock and
	 * error out properly.
	 */
	WaitForLockers(heaplocktag, ShareLock);

	/*
	 * At this moment we are sure that there are no transactions with the
	 * table open for write that don't have this new index in their list of
	 * indexes.  We have waited out all the existing transactions and any new
	 * transaction will have the new index in its list, but the index is still
	 * marked as "not-ready-for-inserts".  The index is consulted while
	 * deciding HOT-safety though.	This arrangement ensures that no new HOT
	 * chains can be created where the new tuple and the old tuple in the
	 * chain have different index keys.
	 *
	 * We now take a new snapshot, and build the index using all tuples that
	 * are visible in this snapshot.  We can be sure that any HOT updates to
	 * these tuples will be compatible with the index, since any updates made
	 * by transactions that didn't know about the index are now committed or
	 * rolled back.  Thus, each visible tuple is either the end of its
	 * HOT-chain or the extension of the chain is HOT-safe for this index.
	 */

	/* Open and lock the parent heap relation */
	rel = heap_openrv(stmt->relation, ShareUpdateExclusiveLock);

	/* And the target index relation */
	indexRelation = index_open(indexRelationId, RowExclusiveLock);

	/* Set ActiveSnapshot since functions in the indexes may need it */
	PushActiveSnapshot(GetTransactionSnapshot());

	/* We have to re-build the IndexInfo struct, since it was lost in commit */
	indexInfo = BuildIndexInfo(indexRelation);
	Assert(!indexInfo->ii_ReadyForInserts);
	indexInfo->ii_Concurrent = true;
	indexInfo->ii_BrokenHotChain = false;

	/* Now build the index */
	index_build(rel, indexRelation, indexInfo, stmt->primary, false);

	/* Close both the relations, but keep the locks */
	heap_close(rel, NoLock);
	index_close(indexRelation, NoLock);

	/*
	 * Update the pg_index row to mark the index as ready for inserts. Once we
	 * commit this transaction, any new transactions that open the table must
	 * insert new entries into the index for insertions and non-HOT updates.
	 */
	index_set_state_flags(indexRelationId, INDEX_CREATE_SET_READY);

	/* we can do away with our snapshot */
	PopActiveSnapshot();

	/*
	 * Commit this transaction to make the indisready update visible.
	 */
	CommitTransactionCommand();
	StartTransactionCommand();

	/*
	 * Phase 3 of concurrent index build
	 *
	 * We once again wait until no transaction can have the table open with
	 * the index marked as read-only for updates.
	 */
	WaitForLockers(heaplocktag, ShareLock);

	/*
	 * Now take the "reference snapshot" that will be used by validate_index()
	 * to filter candidate tuples.	Beware!  There might still be snapshots in
	 * use that treat some transaction as in-progress that our reference
	 * snapshot treats as committed.  If such a recently-committed transaction
	 * deleted tuples in the table, we will not include them in the index; yet
	 * those transactions which see the deleting one as still-in-progress will
	 * expect such tuples to be there once we mark the index as valid.
	 *
	 * We solve this by waiting for all endangered transactions to exit before
	 * we mark the index as valid.
	 *
	 * We also set ActiveSnapshot to this snap, since functions in indexes may
	 * need a snapshot.
	 */
	snapshot = RegisterSnapshot(GetTransactionSnapshot());
	PushActiveSnapshot(snapshot);

	/*
	 * Scan the index and the heap, insert any missing index entries.
	 */
	validate_index(relationId, indexRelationId, snapshot);

	/*
	 * Drop the reference snapshot.  We must do this before waiting out other
	 * snapshot holders, else we will deadlock against other processes also
	 * doing CREATE INDEX CONCURRENTLY, which would see our snapshot as one
	 * they must wait for.	But first, save the snapshot's xmin to use as
	 * limitXmin for GetCurrentVirtualXIDs().
	 */
	limitXmin = snapshot->xmin;

	PopActiveSnapshot();
	UnregisterSnapshot(snapshot);

	/*
	 * The index is now valid in the sense that it contains all currently
	 * interesting tuples.	But since it might not contain tuples deleted just
	 * before the reference snap was taken, we have to wait out any
	 * transactions that might have older snapshots.  Obtain a list of VXIDs
	 * of such transactions, and wait for them individually.
	 *
	 * We can exclude any running transactions that have xmin > the xmin of
	 * our reference snapshot; their oldest snapshot must be newer than ours.
	 * We can also exclude any transactions that have xmin = zero, since they
	 * evidently have no live snapshot at all (and any one they might be in
	 * process of taking is certainly newer than ours).  Transactions in other
	 * DBs can be ignored too, since they'll never even be able to see this
	 * index.
	 *
	 * We can also exclude autovacuum processes and processes running manual
	 * lazy VACUUMs, because they won't be fazed by missing index entries
	 * either.	(Manual ANALYZEs, however, can't be excluded because they
	 * might be within transactions that are going to do arbitrary operations
	 * later.)
	 *
	 * Also, GetCurrentVirtualXIDs never reports our own vxid, so we need not
	 * check for that.
	 *
	 * If a process goes idle-in-transaction with xmin zero, we do not need to
	 * wait for it anymore, per the above argument.  We do not have the
	 * infrastructure right now to stop waiting if that happens, but we can at
	 * least avoid the folly of waiting when it is idle at the time we would
	 * begin to wait.  We do this by repeatedly rechecking the output of
	 * GetCurrentVirtualXIDs.  If, during any iteration, a particular vxid
	 * doesn't show up in the output, we know we can forget about it.
	 */
	old_snapshots = GetCurrentVirtualXIDs(limitXmin, true, false,
										  PROC_IS_AUTOVACUUM | PROC_IN_VACUUM,
										  &n_old_snapshots);

	for (i = 0; i < n_old_snapshots; i++)
	{
		if (!VirtualTransactionIdIsValid(old_snapshots[i]))
			continue;			/* found uninteresting in previous cycle */

		if (i > 0)
		{
			/* see if anything's changed ... */
			VirtualTransactionId *newer_snapshots;
			int			n_newer_snapshots;
			int			j;
			int			k;

			newer_snapshots = GetCurrentVirtualXIDs(limitXmin,
													true, false,
										 PROC_IS_AUTOVACUUM | PROC_IN_VACUUM,
													&n_newer_snapshots);
			for (j = i; j < n_old_snapshots; j++)
			{
				if (!VirtualTransactionIdIsValid(old_snapshots[j]))
					continue;	/* found uninteresting in previous cycle */
				for (k = 0; k < n_newer_snapshots; k++)
				{
					if (VirtualTransactionIdEquals(old_snapshots[j],
												   newer_snapshots[k]))
						break;
				}
				if (k >= n_newer_snapshots)		/* not there anymore */
					SetInvalidVirtualTransactionId(old_snapshots[j]);
			}
			pfree(newer_snapshots);
		}

		if (VirtualTransactionIdIsValid(old_snapshots[i]))
			VirtualXactLock(old_snapshots[i], true);
	}

	/*
	 * Index can now be marked valid -- update its pg_index entry
	 */
	index_set_state_flags(indexRelationId, INDEX_CREATE_SET_VALID);

	/*
	 * The pg_index update will cause backends (including this one) to update
	 * relcache entries for the index itself, but we should also send a
	 * relcache inval on the parent table to force replanning of cached plans.
	 * Otherwise existing sessions might fail to use the new index where it
	 * would be useful.  (Note that our earlier commits did not create reasons
	 * to replan; so relcache flush on the index itself was sufficient.)
	 */
	CacheInvalidateRelcacheByRelid(heaprelid.relId);

	/*
	 * Last thing to do is release the session-level lock on the parent table.
	 */
	UnlockRelationIdForSession(&heaprelid, ShareUpdateExclusiveLock);

	return indexRelationId;
}