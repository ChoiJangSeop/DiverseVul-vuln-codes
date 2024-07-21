ExecWithCheckOptions(ResultRelInfo *resultRelInfo,
					 TupleTableSlot *slot, EState *estate)
{
	ExprContext *econtext;
	ListCell   *l1,
			   *l2;

	/*
	 * We will use the EState's per-tuple context for evaluating constraint
	 * expressions (creating it if it's not already there).
	 */
	econtext = GetPerTupleExprContext(estate);

	/* Arrange for econtext's scan tuple to be the tuple under test */
	econtext->ecxt_scantuple = slot;

	/* Check each of the constraints */
	forboth(l1, resultRelInfo->ri_WithCheckOptions,
			l2, resultRelInfo->ri_WithCheckOptionExprs)
	{
		WithCheckOption *wco = (WithCheckOption *) lfirst(l1);
		ExprState  *wcoExpr = (ExprState *) lfirst(l2);

		/*
		 * WITH CHECK OPTION checks are intended to ensure that the new tuple
		 * is visible (in the case of a view) or that it passes the
		 * 'with-check' policy (in the case of row security).
		 * If the qual evaluates to NULL or FALSE, then the new tuple won't be
		 * included in the view or doesn't pass the 'with-check' policy for the
		 * table.  We need ExecQual to return FALSE for NULL to handle the view
		 * case (the opposite of what we do above for CHECK constraints).
		 */
		if (!ExecQual((List *) wcoExpr, econtext, false))
			ereport(ERROR,
					(errcode(ERRCODE_WITH_CHECK_OPTION_VIOLATION),
				 errmsg("new row violates WITH CHECK OPTION for \"%s\"",
						wco->viewname),
					 errdetail("Failing row contains %s.",
							   ExecBuildSlotValueDescription(slot,
							RelationGetDescr(resultRelInfo->ri_RelationDesc),
															 64))));
	}
}