check_exclusion_constraint(Relation heap, Relation index, IndexInfo *indexInfo,
						   ItemPointer tupleid, Datum *values, bool *isnull,
						   EState *estate, bool newIndex, bool errorOK)
{
	Oid		   *constr_procs = indexInfo->ii_ExclusionProcs;
	uint16	   *constr_strats = indexInfo->ii_ExclusionStrats;
	Oid		   *index_collations = index->rd_indcollation;
	int			index_natts = index->rd_index->indnatts;
	IndexScanDesc index_scan;
	HeapTuple	tup;
	ScanKeyData scankeys[INDEX_MAX_KEYS];
	SnapshotData DirtySnapshot;
	int			i;
	bool		conflict;
	bool		found_self;
	ExprContext *econtext;
	TupleTableSlot *existing_slot;
	TupleTableSlot *save_scantuple;

	/*
	 * If any of the input values are NULL, the constraint check is assumed to
	 * pass (i.e., we assume the operators are strict).
	 */
	for (i = 0; i < index_natts; i++)
	{
		if (isnull[i])
			return true;
	}

	/*
	 * Search the tuples that are in the index for any violations, including
	 * tuples that aren't visible yet.
	 */
	InitDirtySnapshot(DirtySnapshot);

	for (i = 0; i < index_natts; i++)
	{
		ScanKeyEntryInitialize(&scankeys[i],
							   0,
							   i + 1,
							   constr_strats[i],
							   InvalidOid,
							   index_collations[i],
							   constr_procs[i],
							   values[i]);
	}

	/*
	 * Need a TupleTableSlot to put existing tuples in.
	 *
	 * To use FormIndexDatum, we have to make the econtext's scantuple point
	 * to this slot.  Be sure to save and restore caller's value for
	 * scantuple.
	 */
	existing_slot = MakeSingleTupleTableSlot(RelationGetDescr(heap));

	econtext = GetPerTupleExprContext(estate);
	save_scantuple = econtext->ecxt_scantuple;
	econtext->ecxt_scantuple = existing_slot;

	/*
	 * May have to restart scan from this point if a potential conflict is
	 * found.
	 */
retry:
	conflict = false;
	found_self = false;
	index_scan = index_beginscan(heap, index, &DirtySnapshot, index_natts, 0);
	index_rescan(index_scan, scankeys, index_natts, NULL, 0);

	while ((tup = index_getnext(index_scan,
								ForwardScanDirection)) != NULL)
	{
		TransactionId xwait;
		Datum		existing_values[INDEX_MAX_KEYS];
		bool		existing_isnull[INDEX_MAX_KEYS];
		char	   *error_new;
		char	   *error_existing;

		/*
		 * Ignore the entry for the tuple we're trying to check.
		 */
		if (ItemPointerEquals(tupleid, &tup->t_self))
		{
			if (found_self)		/* should not happen */
				elog(ERROR, "found self tuple multiple times in index \"%s\"",
					 RelationGetRelationName(index));
			found_self = true;
			continue;
		}

		/*
		 * Extract the index column values and isnull flags from the existing
		 * tuple.
		 */
		ExecStoreTuple(tup, existing_slot, InvalidBuffer, false);
		FormIndexDatum(indexInfo, existing_slot, estate,
					   existing_values, existing_isnull);

		/* If lossy indexscan, must recheck the condition */
		if (index_scan->xs_recheck)
		{
			if (!index_recheck_constraint(index,
										  constr_procs,
										  existing_values,
										  existing_isnull,
										  values))
				continue;		/* tuple doesn't actually match, so no
								 * conflict */
		}

		/*
		 * At this point we have either a conflict or a potential conflict. If
		 * we're not supposed to raise error, just return the fact of the
		 * potential conflict without waiting to see if it's real.
		 */
		if (errorOK)
		{
			conflict = true;
			break;
		}

		/*
		 * If an in-progress transaction is affecting the visibility of this
		 * tuple, we need to wait for it to complete and then recheck.  For
		 * simplicity we do rechecking by just restarting the whole scan ---
		 * this case probably doesn't happen often enough to be worth trying
		 * harder, and anyway we don't want to hold any index internal locks
		 * while waiting.
		 */
		xwait = TransactionIdIsValid(DirtySnapshot.xmin) ?
			DirtySnapshot.xmin : DirtySnapshot.xmax;

		if (TransactionIdIsValid(xwait))
		{
			index_endscan(index_scan);
			XactLockTableWait(xwait, heap, &tup->t_data->t_ctid,
							  XLTW_RecheckExclusionConstr);
			goto retry;
		}

		/*
		 * We have a definite conflict.  Report it.
		 */
		error_new = BuildIndexValueDescription(index, values, isnull);
		error_existing = BuildIndexValueDescription(index, existing_values,
													existing_isnull);
		if (newIndex)
			ereport(ERROR,
					(errcode(ERRCODE_EXCLUSION_VIOLATION),
					 errmsg("could not create exclusion constraint \"%s\"",
							RelationGetRelationName(index)),
					 errdetail("Key %s conflicts with key %s.",
							   error_new, error_existing),
					 errtableconstraint(heap,
										RelationGetRelationName(index))));
		else
			ereport(ERROR,
					(errcode(ERRCODE_EXCLUSION_VIOLATION),
					 errmsg("conflicting key value violates exclusion constraint \"%s\"",
							RelationGetRelationName(index)),
					 errdetail("Key %s conflicts with existing key %s.",
							   error_new, error_existing),
					 errtableconstraint(heap,
										RelationGetRelationName(index))));
	}

	index_endscan(index_scan);

	/*
	 * Ordinarily, at this point the search should have found the originally
	 * inserted tuple, unless we exited the loop early because of conflict.
	 * However, it is possible to define exclusion constraints for which that
	 * wouldn't be true --- for instance, if the operator is <>. So we no
	 * longer complain if found_self is still false.
	 */

	econtext->ecxt_scantuple = save_scantuple;

	ExecDropSingleTupleTableSlot(existing_slot);

	return !conflict;
}