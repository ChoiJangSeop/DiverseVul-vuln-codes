diff_mark_adjust_tp(
    tabpage_T	*tp,
    int		idx,
    linenr_T	line1,
    linenr_T	line2,
    long	amount,
    long	amount_after)
{
    diff_T	*dp;
    diff_T	*dprev;
    diff_T	*dnext;
    int		i;
    int		inserted, deleted;
    int		n, off;
    linenr_T	last;
    linenr_T	lnum_deleted = line1;	// lnum of remaining deletion
    int		check_unchanged;

    if (diff_internal())
    {
	// Will update diffs before redrawing.  Set _invalid to update the
	// diffs themselves, set _update to also update folds properly just
	// before redrawing.
	// Do update marks here, it is needed for :%diffput.
	tp->tp_diff_invalid = TRUE;
	tp->tp_diff_update = TRUE;
    }

    if (line2 == MAXLNUM)
    {
	// mark_adjust(99, MAXLNUM, 9, 0): insert lines
	inserted = amount;
	deleted = 0;
    }
    else if (amount_after > 0)
    {
	// mark_adjust(99, 98, MAXLNUM, 9): a change that inserts lines
	inserted = amount_after;
	deleted = 0;
    }
    else
    {
	// mark_adjust(98, 99, MAXLNUM, -2): delete lines
	inserted = 0;
	deleted = -amount_after;
    }

    dprev = NULL;
    dp = tp->tp_first_diff;
    for (;;)
    {
	// If the change is after the previous diff block and before the next
	// diff block, thus not touching an existing change, create a new diff
	// block.  Don't do this when ex_diffgetput() is busy.
	if ((dp == NULL || dp->df_lnum[idx] - 1 > line2
		    || (line2 == MAXLNUM && dp->df_lnum[idx] > line1))
		&& (dprev == NULL
		    || dprev->df_lnum[idx] + dprev->df_count[idx] < line1)
		&& !diff_busy)
	{
	    dnext = diff_alloc_new(tp, dprev, dp);
	    if (dnext == NULL)
		return;

	    dnext->df_lnum[idx] = line1;
	    dnext->df_count[idx] = inserted;
	    for (i = 0; i < DB_COUNT; ++i)
		if (tp->tp_diffbuf[i] != NULL && i != idx)
		{
		    if (dprev == NULL)
			dnext->df_lnum[i] = line1;
		    else
			dnext->df_lnum[i] = line1
			    + (dprev->df_lnum[i] + dprev->df_count[i])
			    - (dprev->df_lnum[idx] + dprev->df_count[idx]);
		    dnext->df_count[i] = deleted;
		}
	}

	// if at end of the list, quit
	if (dp == NULL)
	    break;

	/*
	 * Check for these situations:
	 *	  1  2	3
	 *	  1  2	3
	 * line1     2	3  4  5
	 *	     2	3  4  5
	 *	     2	3  4  5
	 * line2     2	3  4  5
	 *		3     5  6
	 *		3     5  6
	 */
	// compute last line of this change
	last = dp->df_lnum[idx] + dp->df_count[idx] - 1;

	// 1. change completely above line1: nothing to do
	if (last >= line1 - 1)
	{
	    // 6. change below line2: only adjust for amount_after; also when
	    // "deleted" became zero when deleted all lines between two diffs
	    if (dp->df_lnum[idx] - (deleted + inserted != 0) > line2)
	    {
		if (amount_after == 0)
		    break;	// nothing left to change
		dp->df_lnum[idx] += amount_after;
	    }
	    else
	    {
		check_unchanged = FALSE;

		// 2. 3. 4. 5.: inserted/deleted lines touching this diff.
		if (deleted > 0)
		{
		    off = 0;
		    if (dp->df_lnum[idx] >= line1)
		    {
			if (last <= line2)
			{
			    // 4. delete all lines of diff
			    if (dp->df_next != NULL
				    && dp->df_next->df_lnum[idx] - 1 <= line2)
			    {
				// delete continues in next diff, only do
				// lines until that one
				n = dp->df_next->df_lnum[idx] - lnum_deleted;
				deleted -= n;
				n -= dp->df_count[idx];
				lnum_deleted = dp->df_next->df_lnum[idx];
			    }
			    else
				n = deleted - dp->df_count[idx];
			    dp->df_count[idx] = 0;
			}
			else
			{
			    // 5. delete lines at or just before top of diff
			    off = dp->df_lnum[idx] - lnum_deleted;
			    n = off;
			    dp->df_count[idx] -= line2 - dp->df_lnum[idx] + 1;
			    check_unchanged = TRUE;
			}
			dp->df_lnum[idx] = line1;
		    }
		    else
		    {
			if (last < line2)
			{
			    // 2. delete at end of diff
			    dp->df_count[idx] -= last - lnum_deleted + 1;
			    if (dp->df_next != NULL
				    && dp->df_next->df_lnum[idx] - 1 <= line2)
			    {
				// delete continues in next diff, only do
				// lines until that one
				n = dp->df_next->df_lnum[idx] - 1 - last;
				deleted -= dp->df_next->df_lnum[idx]
							       - lnum_deleted;
				lnum_deleted = dp->df_next->df_lnum[idx];
			    }
			    else
				n = line2 - last;
			    check_unchanged = TRUE;
			}
			else
			{
			    // 3. delete lines inside the diff
			    n = 0;
			    dp->df_count[idx] -= deleted;
			}
		    }

		    for (i = 0; i < DB_COUNT; ++i)
			if (tp->tp_diffbuf[i] != NULL && i != idx)
			{
			    dp->df_lnum[i] -= off;
			    dp->df_count[i] += n;
			}
		}
		else
		{
		    if (dp->df_lnum[idx] <= line1)
		    {
			// inserted lines somewhere in this diff
			dp->df_count[idx] += inserted;
			check_unchanged = TRUE;
		    }
		    else
			// inserted lines somewhere above this diff
			dp->df_lnum[idx] += inserted;
		}

		if (check_unchanged)
		    // Check if inserted lines are equal, may reduce the
		    // size of the diff.  TODO: also check for equal lines
		    // in the middle and perhaps split the block.
		    diff_check_unchanged(tp, dp);
	    }
	}

	// check if this block touches the previous one, may merge them.
	if (dprev != NULL && dprev->df_lnum[idx] + dprev->df_count[idx]
							  == dp->df_lnum[idx])
	{
	    for (i = 0; i < DB_COUNT; ++i)
		if (tp->tp_diffbuf[i] != NULL)
		    dprev->df_count[i] += dp->df_count[i];
	    dprev->df_next = dp->df_next;
	    vim_free(dp);
	    dp = dprev->df_next;
	}
	else
	{
	    // Advance to next entry.
	    dprev = dp;
	    dp = dp->df_next;
	}
    }

    dprev = NULL;
    dp = tp->tp_first_diff;
    while (dp != NULL)
    {
	// All counts are zero, remove this entry.
	for (i = 0; i < DB_COUNT; ++i)
	    if (tp->tp_diffbuf[i] != NULL && dp->df_count[i] != 0)
		break;
	if (i == DB_COUNT)
	{
	    dnext = dp->df_next;
	    vim_free(dp);
	    dp = dnext;
	    if (dprev == NULL)
		tp->tp_first_diff = dnext;
	    else
		dprev->df_next = dnext;
	}
	else
	{
	    // Advance to next entry.
	    dprev = dp;
	    dp = dp->df_next;
	}

    }

    if (tp == curtab)
    {
	// Don't redraw right away, this updates the diffs, which can be slow.
	need_diff_redraw = TRUE;

	// Need to recompute the scroll binding, may remove or add filler
	// lines (e.g., when adding lines above w_topline). But it's slow when
	// making many changes, postpone until redrawing.
	diff_need_scrollbind = TRUE;
    }
}