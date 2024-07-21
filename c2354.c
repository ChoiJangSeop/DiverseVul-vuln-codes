_bt_check_unique(Relation rel, IndexTuple itup, Relation heapRel,
				 Buffer buf, OffsetNumber offset, ScanKey itup_scankey,
				 IndexUniqueCheck checkUnique, bool *is_unique)
{
	TupleDesc	itupdesc = RelationGetDescr(rel);
	int			natts = rel->rd_rel->relnatts;
	SnapshotData SnapshotDirty;
	OffsetNumber maxoff;
	Page		page;
	BTPageOpaque opaque;
	Buffer		nbuf = InvalidBuffer;
	bool		found = false;

	/* Assume unique until we find a duplicate */
	*is_unique = true;

	InitDirtySnapshot(SnapshotDirty);

	page = BufferGetPage(buf);
	opaque = (BTPageOpaque) PageGetSpecialPointer(page);
	maxoff = PageGetMaxOffsetNumber(page);

	/*
	 * Scan over all equal tuples, looking for live conflicts.
	 */
	for (;;)
	{
		ItemId		curitemid;
		IndexTuple	curitup;
		BlockNumber nblkno;

		/*
		 * make sure the offset points to an actual item before trying to
		 * examine it...
		 */
		if (offset <= maxoff)
		{
			curitemid = PageGetItemId(page, offset);

			/*
			 * We can skip items that are marked killed.
			 *
			 * Formerly, we applied _bt_isequal() before checking the kill
			 * flag, so as to fall out of the item loop as soon as possible.
			 * However, in the presence of heavy update activity an index may
			 * contain many killed items with the same key; running
			 * _bt_isequal() on each killed item gets expensive. Furthermore
			 * it is likely that the non-killed version of each key appears
			 * first, so that we didn't actually get to exit any sooner
			 * anyway. So now we just advance over killed items as quickly as
			 * we can. We only apply _bt_isequal() when we get to a non-killed
			 * item or the end of the page.
			 */
			if (!ItemIdIsDead(curitemid))
			{
				ItemPointerData htid;
				bool		all_dead;

				/*
				 * _bt_compare returns 0 for (1,NULL) and (1,NULL) - this's
				 * how we handling NULLs - and so we must not use _bt_compare
				 * in real comparison, but only for ordering/finding items on
				 * pages. - vadim 03/24/97
				 */
				if (!_bt_isequal(itupdesc, page, offset, natts, itup_scankey))
					break;		/* we're past all the equal tuples */

				/* okay, we gotta fetch the heap tuple ... */
				curitup = (IndexTuple) PageGetItem(page, curitemid);
				htid = curitup->t_tid;

				/*
				 * If we are doing a recheck, we expect to find the tuple we
				 * are rechecking.  It's not a duplicate, but we have to keep
				 * scanning.
				 */
				if (checkUnique == UNIQUE_CHECK_EXISTING &&
					ItemPointerCompare(&htid, &itup->t_tid) == 0)
				{
					found = true;
				}

				/*
				 * We check the whole HOT-chain to see if there is any tuple
				 * that satisfies SnapshotDirty.  This is necessary because we
				 * have just a single index entry for the entire chain.
				 */
				else if (heap_hot_search(&htid, heapRel, &SnapshotDirty,
										 &all_dead))
				{
					TransactionId xwait;

					/*
					 * It is a duplicate. If we are only doing a partial
					 * check, then don't bother checking if the tuple is being
					 * updated in another transaction. Just return the fact
					 * that it is a potential conflict and leave the full
					 * check till later.
					 */
					if (checkUnique == UNIQUE_CHECK_PARTIAL)
					{
						if (nbuf != InvalidBuffer)
							_bt_relbuf(rel, nbuf);
						*is_unique = false;
						return InvalidTransactionId;
					}

					/*
					 * If this tuple is being updated by other transaction
					 * then we have to wait for its commit/abort.
					 */
					xwait = (TransactionIdIsValid(SnapshotDirty.xmin)) ?
						SnapshotDirty.xmin : SnapshotDirty.xmax;

					if (TransactionIdIsValid(xwait))
					{
						if (nbuf != InvalidBuffer)
							_bt_relbuf(rel, nbuf);
						/* Tell _bt_doinsert to wait... */
						return xwait;
					}

					/*
					 * Otherwise we have a definite conflict.  But before
					 * complaining, look to see if the tuple we want to insert
					 * is itself now committed dead --- if so, don't complain.
					 * This is a waste of time in normal scenarios but we must
					 * do it to support CREATE INDEX CONCURRENTLY.
					 *
					 * We must follow HOT-chains here because during
					 * concurrent index build, we insert the root TID though
					 * the actual tuple may be somewhere in the HOT-chain.
					 * While following the chain we might not stop at the
					 * exact tuple which triggered the insert, but that's OK
					 * because if we find a live tuple anywhere in this chain,
					 * we have a unique key conflict.  The other live tuple is
					 * not part of this chain because it had a different index
					 * entry.
					 */
					htid = itup->t_tid;
					if (heap_hot_search(&htid, heapRel, SnapshotSelf, NULL))
					{
						/* Normal case --- it's still live */
					}
					else
					{
						/*
						 * It's been deleted, so no error, and no need to
						 * continue searching
						 */
						break;
					}

					/*
					 * This is a definite conflict.  Break the tuple down into
					 * datums and report the error.  But first, make sure we
					 * release the buffer locks we're holding ---
					 * BuildIndexValueDescription could make catalog accesses,
					 * which in the worst case might touch this same index and
					 * cause deadlocks.
					 */
					if (nbuf != InvalidBuffer)
						_bt_relbuf(rel, nbuf);
					_bt_relbuf(rel, buf);

					{
						Datum		values[INDEX_MAX_KEYS];
						bool		isnull[INDEX_MAX_KEYS];

						index_deform_tuple(itup, RelationGetDescr(rel),
										   values, isnull);
						ereport(ERROR,
								(errcode(ERRCODE_UNIQUE_VIOLATION),
								 errmsg("duplicate key value violates unique constraint \"%s\"",
										RelationGetRelationName(rel)),
								 errdetail("Key %s already exists.",
										   BuildIndexValueDescription(rel,
															values, isnull)),
								 errtableconstraint(heapRel,
											 RelationGetRelationName(rel))));
					}
				}
				else if (all_dead)
				{
					/*
					 * The conflicting tuple (or whole HOT chain) is dead to
					 * everyone, so we may as well mark the index entry
					 * killed.
					 */
					ItemIdMarkDead(curitemid);
					opaque->btpo_flags |= BTP_HAS_GARBAGE;

					/*
					 * Mark buffer with a dirty hint, since state is not
					 * crucial. Be sure to mark the proper buffer dirty.
					 */
					if (nbuf != InvalidBuffer)
						MarkBufferDirtyHint(nbuf, true);
					else
						MarkBufferDirtyHint(buf, true);
				}
			}
		}

		/*
		 * Advance to next tuple to continue checking.
		 */
		if (offset < maxoff)
			offset = OffsetNumberNext(offset);
		else
		{
			/* If scankey == hikey we gotta check the next page too */
			if (P_RIGHTMOST(opaque))
				break;
			if (!_bt_isequal(itupdesc, page, P_HIKEY,
							 natts, itup_scankey))
				break;
			/* Advance to next non-dead page --- there must be one */
			for (;;)
			{
				nblkno = opaque->btpo_next;
				nbuf = _bt_relandgetbuf(rel, nbuf, nblkno, BT_READ);
				page = BufferGetPage(nbuf);
				opaque = (BTPageOpaque) PageGetSpecialPointer(page);
				if (!P_IGNORE(opaque))
					break;
				if (P_RIGHTMOST(opaque))
					elog(ERROR, "fell off the end of index \"%s\"",
						 RelationGetRelationName(rel));
			}
			maxoff = PageGetMaxOffsetNumber(page);
			offset = P_FIRSTDATAKEY(opaque);
		}
	}

	/*
	 * If we are doing a recheck then we should have found the tuple we are
	 * checking.  Otherwise there's something very wrong --- probably, the
	 * index is on a non-immutable expression.
	 */
	if (checkUnique == UNIQUE_CHECK_EXISTING && !found)
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("failed to re-find tuple within index \"%s\"",
						RelationGetRelationName(rel)),
		 errhint("This may be because of a non-immutable index expression."),
				 errtableconstraint(heapRel,
									RelationGetRelationName(rel))));

	if (nbuf != InvalidBuffer)
		_bt_relbuf(rel, nbuf);

	return InvalidTransactionId;
}