CopyFrom(CopyState cstate)
{
	HeapTuple	tuple;
	TupleDesc	tupDesc;
	Datum	   *values;
	bool	   *nulls;
	ResultRelInfo *resultRelInfo;
	EState	   *estate = CreateExecutorState(); /* for ExecConstraints() */
	ExprContext *econtext;
	TupleTableSlot *myslot;
	MemoryContext oldcontext = CurrentMemoryContext;

	ErrorContextCallback errcallback;
	CommandId	mycid = GetCurrentCommandId(true);
	int			hi_options = 0; /* start with default heap_insert options */
	BulkInsertState bistate;
	uint64		processed = 0;
	bool		useHeapMultiInsert;
	int			nBufferedTuples = 0;

#define MAX_BUFFERED_TUPLES 1000
	HeapTuple  *bufferedTuples = NULL;	/* initialize to silence warning */
	Size		bufferedTuplesSize = 0;
	int			firstBufferedLineNo = 0;

	Assert(cstate->rel);

	if (cstate->rel->rd_rel->relkind != RELKIND_RELATION)
	{
		if (cstate->rel->rd_rel->relkind == RELKIND_VIEW)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot copy to view \"%s\"",
							RelationGetRelationName(cstate->rel))));
		else if (cstate->rel->rd_rel->relkind == RELKIND_MATVIEW)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot copy to materialized view \"%s\"",
							RelationGetRelationName(cstate->rel))));
		else if (cstate->rel->rd_rel->relkind == RELKIND_FOREIGN_TABLE)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot copy to foreign table \"%s\"",
							RelationGetRelationName(cstate->rel))));
		else if (cstate->rel->rd_rel->relkind == RELKIND_SEQUENCE)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot copy to sequence \"%s\"",
							RelationGetRelationName(cstate->rel))));
		else
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("cannot copy to non-table relation \"%s\"",
							RelationGetRelationName(cstate->rel))));
	}

	tupDesc = RelationGetDescr(cstate->rel);

	/*----------
	 * Check to see if we can avoid writing WAL
	 *
	 * If archive logging/streaming is not enabled *and* either
	 *	- table was created in same transaction as this COPY
	 *	- data is being written to relfilenode created in this transaction
	 * then we can skip writing WAL.  It's safe because if the transaction
	 * doesn't commit, we'll discard the table (or the new relfilenode file).
	 * If it does commit, we'll have done the heap_sync at the bottom of this
	 * routine first.
	 *
	 * As mentioned in comments in utils/rel.h, the in-same-transaction test
	 * is not always set correctly, since in rare cases rd_newRelfilenodeSubid
	 * can be cleared before the end of the transaction. The exact case is
	 * when a relation sets a new relfilenode twice in same transaction, yet
	 * the second one fails in an aborted subtransaction, e.g.
	 *
	 * BEGIN;
	 * TRUNCATE t;
	 * SAVEPOINT save;
	 * TRUNCATE t;
	 * ROLLBACK TO save;
	 * COPY ...
	 *
	 * Also, if the target file is new-in-transaction, we assume that checking
	 * FSM for free space is a waste of time, even if we must use WAL because
	 * of archiving.  This could possibly be wrong, but it's unlikely.
	 *
	 * The comments for heap_insert and RelationGetBufferForTuple specify that
	 * skipping WAL logging is only safe if we ensure that our tuples do not
	 * go into pages containing tuples from any other transactions --- but this
	 * must be the case if we have a new table or new relfilenode, so we need
	 * no additional work to enforce that.
	 *----------
	 */
	/* createSubid is creation check, newRelfilenodeSubid is truncation check */
	if (cstate->rel->rd_createSubid != InvalidSubTransactionId ||
		cstate->rel->rd_newRelfilenodeSubid != InvalidSubTransactionId)
	{
		hi_options |= HEAP_INSERT_SKIP_FSM;
		if (!XLogIsNeeded())
			hi_options |= HEAP_INSERT_SKIP_WAL;
	}

	/*
	 * Optimize if new relfilenode was created in this subxact or one of its
	 * committed children and we won't see those rows later as part of an
	 * earlier scan or command. This ensures that if this subtransaction
	 * aborts then the frozen rows won't be visible after xact cleanup. Note
	 * that the stronger test of exactly which subtransaction created it is
	 * crucial for correctness of this optimisation.
	 */
	if (cstate->freeze)
	{
		if (!ThereAreNoPriorRegisteredSnapshots() || !ThereAreNoReadyPortals())
			ereport(ERROR,
					(ERRCODE_INVALID_TRANSACTION_STATE,
					 errmsg("cannot perform FREEZE because of prior transaction activity")));

		if (cstate->rel->rd_createSubid != GetCurrentSubTransactionId() &&
		 cstate->rel->rd_newRelfilenodeSubid != GetCurrentSubTransactionId())
			ereport(ERROR,
					(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE,
					 errmsg("cannot perform FREEZE because the table was not created or truncated in the current subtransaction")));

		hi_options |= HEAP_INSERT_FROZEN;
	}

	/*
	 * We need a ResultRelInfo so we can use the regular executor's
	 * index-entry-making machinery.  (There used to be a huge amount of code
	 * here that basically duplicated execUtils.c ...)
	 */
	resultRelInfo = makeNode(ResultRelInfo);
	InitResultRelInfo(resultRelInfo,
					  cstate->rel,
					  1,		/* dummy rangetable index */
					  0);

	ExecOpenIndices(resultRelInfo);

	estate->es_result_relations = resultRelInfo;
	estate->es_num_result_relations = 1;
	estate->es_result_relation_info = resultRelInfo;
	estate->es_range_table = cstate->range_table;

	/* Set up a tuple slot too */
	myslot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(myslot, tupDesc);
	/* Triggers might need a slot as well */
	estate->es_trig_tuple_slot = ExecInitExtraTupleSlot(estate);

	/*
	 * It's more efficient to prepare a bunch of tuples for insertion, and
	 * insert them in one heap_multi_insert() call, than call heap_insert()
	 * separately for every tuple. However, we can't do that if there are
	 * BEFORE/INSTEAD OF triggers, or we need to evaluate volatile default
	 * expressions. Such triggers or expressions might query the table we're
	 * inserting to, and act differently if the tuples that have already been
	 * processed and prepared for insertion are not there.
	 */
	if ((resultRelInfo->ri_TrigDesc != NULL &&
		 (resultRelInfo->ri_TrigDesc->trig_insert_before_row ||
		  resultRelInfo->ri_TrigDesc->trig_insert_instead_row)) ||
		cstate->volatile_defexprs)
	{
		useHeapMultiInsert = false;
	}
	else
	{
		useHeapMultiInsert = true;
		bufferedTuples = palloc(MAX_BUFFERED_TUPLES * sizeof(HeapTuple));
	}

	/* Prepare to catch AFTER triggers. */
	AfterTriggerBeginQuery();

	/*
	 * Check BEFORE STATEMENT insertion triggers. It's debatable whether we
	 * should do this for COPY, since it's not really an "INSERT" statement as
	 * such. However, executing these triggers maintains consistency with the
	 * EACH ROW triggers that we already fire on COPY.
	 */
	ExecBSInsertTriggers(estate, resultRelInfo);

	values = (Datum *) palloc(tupDesc->natts * sizeof(Datum));
	nulls = (bool *) palloc(tupDesc->natts * sizeof(bool));

	bistate = GetBulkInsertState();
	econtext = GetPerTupleExprContext(estate);

	/* Set up callback to identify error line number */
	errcallback.callback = CopyFromErrorCallback;
	errcallback.arg = (void *) cstate;
	errcallback.previous = error_context_stack;
	error_context_stack = &errcallback;

	for (;;)
	{
		TupleTableSlot *slot;
		bool		skip_tuple;
		Oid			loaded_oid = InvalidOid;

		CHECK_FOR_INTERRUPTS();

		if (nBufferedTuples == 0)
		{
			/*
			 * Reset the per-tuple exprcontext. We can only do this if the
			 * tuple buffer is empty. (Calling the context the per-tuple
			 * memory context is a bit of a misnomer now.)
			 */
			ResetPerTupleExprContext(estate);
		}

		/* Switch into its memory context */
		MemoryContextSwitchTo(GetPerTupleMemoryContext(estate));

		if (!NextCopyFrom(cstate, econtext, values, nulls, &loaded_oid))
			break;

		/* And now we can form the input tuple. */
		tuple = heap_form_tuple(tupDesc, values, nulls);

		if (loaded_oid != InvalidOid)
			HeapTupleSetOid(tuple, loaded_oid);

		/*
		 * Constraints might reference the tableoid column, so initialize
		 * t_tableOid before evaluating them.
		 */
		tuple->t_tableOid = RelationGetRelid(resultRelInfo->ri_RelationDesc);

		/* Triggers and stuff need to be invoked in query context. */
		MemoryContextSwitchTo(oldcontext);

		/* Place tuple in tuple slot --- but slot shouldn't free it */
		slot = myslot;
		ExecStoreTuple(tuple, slot, InvalidBuffer, false);

		skip_tuple = false;

		/* BEFORE ROW INSERT Triggers */
		if (resultRelInfo->ri_TrigDesc &&
			resultRelInfo->ri_TrigDesc->trig_insert_before_row)
		{
			slot = ExecBRInsertTriggers(estate, resultRelInfo, slot);

			if (slot == NULL)	/* "do nothing" */
				skip_tuple = true;
			else	/* trigger might have changed tuple */
				tuple = ExecMaterializeSlot(slot);
		}

		if (!skip_tuple)
		{
			/* Check the constraints of the tuple */
			if (cstate->rel->rd_att->constr)
				ExecConstraints(resultRelInfo, slot, estate);

			if (useHeapMultiInsert)
			{
				/* Add this tuple to the tuple buffer */
				if (nBufferedTuples == 0)
					firstBufferedLineNo = cstate->cur_lineno;
				bufferedTuples[nBufferedTuples++] = tuple;
				bufferedTuplesSize += tuple->t_len;

				/*
				 * If the buffer filled up, flush it. Also flush if the total
				 * size of all the tuples in the buffer becomes large, to
				 * avoid using large amounts of memory for the buffers when
				 * the tuples are exceptionally wide.
				 */
				if (nBufferedTuples == MAX_BUFFERED_TUPLES ||
					bufferedTuplesSize > 65535)
				{
					CopyFromInsertBatch(cstate, estate, mycid, hi_options,
										resultRelInfo, myslot, bistate,
										nBufferedTuples, bufferedTuples,
										firstBufferedLineNo);
					nBufferedTuples = 0;
					bufferedTuplesSize = 0;
				}
			}
			else
			{
				List	   *recheckIndexes = NIL;

				/* OK, store the tuple and create index entries for it */
				heap_insert(cstate->rel, tuple, mycid, hi_options, bistate);

				if (resultRelInfo->ri_NumIndices > 0)
					recheckIndexes = ExecInsertIndexTuples(slot, &(tuple->t_self),
														   estate);

				/* AFTER ROW INSERT Triggers */
				ExecARInsertTriggers(estate, resultRelInfo, tuple,
									 recheckIndexes);

				list_free(recheckIndexes);
			}

			/*
			 * We count only tuples not suppressed by a BEFORE INSERT trigger;
			 * this is the same definition used by execMain.c for counting
			 * tuples inserted by an INSERT command.
			 */
			processed++;
		}
	}

	/* Flush any remaining buffered tuples */
	if (nBufferedTuples > 0)
		CopyFromInsertBatch(cstate, estate, mycid, hi_options,
							resultRelInfo, myslot, bistate,
							nBufferedTuples, bufferedTuples,
							firstBufferedLineNo);

	/* Done, clean up */
	error_context_stack = errcallback.previous;

	FreeBulkInsertState(bistate);

	MemoryContextSwitchTo(oldcontext);

	/* Execute AFTER STATEMENT insertion triggers */
	ExecASInsertTriggers(estate, resultRelInfo);

	/* Handle queued AFTER triggers */
	AfterTriggerEndQuery(estate);

	pfree(values);
	pfree(nulls);

	ExecResetTupleTable(estate->es_tupleTable, false);

	ExecCloseIndices(resultRelInfo);

	FreeExecutorState(estate);

	/*
	 * If we skipped writing WAL, then we need to sync the heap (but not
	 * indexes since those use WAL anyway)
	 */
	if (hi_options & HEAP_INSERT_SKIP_WAL)
		heap_sync(cstate->rel);

	return processed;
}