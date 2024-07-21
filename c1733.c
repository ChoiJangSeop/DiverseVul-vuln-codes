index_constraint_create(Relation heapRelation,
						Oid indexRelationId,
						IndexInfo *indexInfo,
						const char *constraintName,
						char constraintType,
						bool deferrable,
						bool initdeferred,
						bool mark_as_primary,
						bool update_pgindex,
						bool remove_old_dependencies,
						bool allow_system_table_mods,
						bool is_internal)
{
	Oid			namespaceId = RelationGetNamespace(heapRelation);
	ObjectAddress myself,
				referenced;
	Oid			conOid;

	/* constraint creation support doesn't work while bootstrapping */
	Assert(!IsBootstrapProcessingMode());

	/* enforce system-table restriction */
	if (!allow_system_table_mods &&
		IsSystemRelation(heapRelation) &&
		IsNormalProcessingMode())
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("user-defined indexes on system catalog tables are not supported")));

	/* primary/unique constraints shouldn't have any expressions */
	if (indexInfo->ii_Expressions &&
		constraintType != CONSTRAINT_EXCLUSION)
		elog(ERROR, "constraints cannot have index expressions");

	/*
	 * If we're manufacturing a constraint for a pre-existing index, we need
	 * to get rid of the existing auto dependencies for the index (the ones
	 * that index_create() would have made instead of calling this function).
	 *
	 * Note: this code would not necessarily do the right thing if the index
	 * has any expressions or predicate, but we'd never be turning such an
	 * index into a UNIQUE or PRIMARY KEY constraint.
	 */
	if (remove_old_dependencies)
		deleteDependencyRecordsForClass(RelationRelationId, indexRelationId,
										RelationRelationId, DEPENDENCY_AUTO);

	/*
	 * Construct a pg_constraint entry.
	 */
	conOid = CreateConstraintEntry(constraintName,
								   namespaceId,
								   constraintType,
								   deferrable,
								   initdeferred,
								   true,
								   RelationGetRelid(heapRelation),
								   indexInfo->ii_KeyAttrNumbers,
								   indexInfo->ii_NumIndexAttrs,
								   InvalidOid,	/* no domain */
								   indexRelationId,		/* index OID */
								   InvalidOid,	/* no foreign key */
								   NULL,
								   NULL,
								   NULL,
								   NULL,
								   0,
								   ' ',
								   ' ',
								   ' ',
								   indexInfo->ii_ExclusionOps,
								   NULL,		/* no check constraint */
								   NULL,
								   NULL,
								   true,		/* islocal */
								   0,	/* inhcount */
								   true,		/* noinherit */
								   is_internal);

	/*
	 * Register the index as internally dependent on the constraint.
	 *
	 * Note that the constraint has a dependency on the table, so we don't
	 * need (or want) any direct dependency from the index to the table.
	 */
	myself.classId = RelationRelationId;
	myself.objectId = indexRelationId;
	myself.objectSubId = 0;

	referenced.classId = ConstraintRelationId;
	referenced.objectId = conOid;
	referenced.objectSubId = 0;

	recordDependencyOn(&myself, &referenced, DEPENDENCY_INTERNAL);

	/*
	 * If the constraint is deferrable, create the deferred uniqueness
	 * checking trigger.  (The trigger will be given an internal dependency on
	 * the constraint by CreateTrigger.)
	 */
	if (deferrable)
	{
		RangeVar   *heapRel;
		CreateTrigStmt *trigger;

		heapRel = makeRangeVar(get_namespace_name(namespaceId),
							   pstrdup(RelationGetRelationName(heapRelation)),
							   -1);

		trigger = makeNode(CreateTrigStmt);
		trigger->trigname = (constraintType == CONSTRAINT_PRIMARY) ?
			"PK_ConstraintTrigger" :
			"Unique_ConstraintTrigger";
		trigger->relation = heapRel;
		trigger->funcname = SystemFuncName("unique_key_recheck");
		trigger->args = NIL;
		trigger->row = true;
		trigger->timing = TRIGGER_TYPE_AFTER;
		trigger->events = TRIGGER_TYPE_INSERT | TRIGGER_TYPE_UPDATE;
		trigger->columns = NIL;
		trigger->whenClause = NULL;
		trigger->isconstraint = true;
		trigger->deferrable = true;
		trigger->initdeferred = initdeferred;
		trigger->constrrel = NULL;

		(void) CreateTrigger(trigger, NULL, conOid, indexRelationId, true);
	}

	/*
	 * If needed, mark the table as having a primary key.  We assume it can't
	 * have been so marked already, so no need to clear the flag in the other
	 * case.
	 *
	 * Note: this might better be done by callers.	We do it here to avoid
	 * exposing index_update_stats() globally, but that wouldn't be necessary
	 * if relhaspkey went away.
	 */
	if (mark_as_primary)
		index_update_stats(heapRelation,
						   true,
						   true,
						   -1.0);

	/*
	 * If needed, mark the index as primary and/or deferred in pg_index.
	 *
	 * Note: When making an existing index into a constraint, caller must
	 * have a table lock that prevents concurrent table updates; otherwise,
	 * there is a risk that concurrent readers of the table will miss seeing
	 * this index at all.
	 */
	if (update_pgindex && (mark_as_primary || deferrable))
	{
		Relation	pg_index;
		HeapTuple	indexTuple;
		Form_pg_index indexForm;
		bool		dirty = false;

		pg_index = heap_open(IndexRelationId, RowExclusiveLock);

		indexTuple = SearchSysCacheCopy1(INDEXRELID,
										 ObjectIdGetDatum(indexRelationId));
		if (!HeapTupleIsValid(indexTuple))
			elog(ERROR, "cache lookup failed for index %u", indexRelationId);
		indexForm = (Form_pg_index) GETSTRUCT(indexTuple);

		if (mark_as_primary && !indexForm->indisprimary)
		{
			indexForm->indisprimary = true;
			dirty = true;
		}

		if (deferrable && indexForm->indimmediate)
		{
			indexForm->indimmediate = false;
			dirty = true;
		}

		if (dirty)
		{
			simple_heap_update(pg_index, &indexTuple->t_self, indexTuple);
			CatalogUpdateIndexes(pg_index, indexTuple);

			InvokeObjectPostAlterHookArg(IndexRelationId, indexRelationId, 0,
										 InvalidOid, is_internal);
		}

		heap_freetuple(indexTuple);
		heap_close(pg_index, RowExclusiveLock);
	}
}