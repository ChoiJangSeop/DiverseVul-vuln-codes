undo_time(
    long	step,
    int		sec,
    int		file,
    int		absolute)
{
    long	    target;
    long	    closest;
    long	    closest_start;
    long	    closest_seq = 0;
    long	    val;
    u_header_T	    *uhp = NULL;
    u_header_T	    *last;
    int		    mark;
    int		    nomark = 0;  // shut up compiler
    int		    round;
    int		    dosec = sec;
    int		    dofile = file;
    int		    above = FALSE;
    int		    did_undo = TRUE;

    // First make sure the current undoable change is synced.
    if (curbuf->b_u_synced == FALSE)
	u_sync(TRUE);

    u_newcount = 0;
    u_oldcount = 0;
    if (curbuf->b_ml.ml_flags & ML_EMPTY)
	u_oldcount = -1;

    // "target" is the node below which we want to be.
    // Init "closest" to a value we can't reach.
    if (absolute)
    {
	target = step;
	closest = -1;
    }
    else
    {
	if (dosec)
	    target = (long)(curbuf->b_u_time_cur) + step;
	else if (dofile)
	{
	    if (step < 0)
	    {
		// Going back to a previous write. If there were changes after
		// the last write, count that as moving one file-write, so
		// that ":earlier 1f" undoes all changes since the last save.
		uhp = curbuf->b_u_curhead;
		if (uhp != NULL)
		    uhp = uhp->uh_next.ptr;
		else
		    uhp = curbuf->b_u_newhead;
		if (uhp != NULL && uhp->uh_save_nr != 0)
		    // "uh_save_nr" was set in the last block, that means
		    // there were no changes since the last write
		    target = curbuf->b_u_save_nr_cur + step;
		else
		    // count the changes since the last write as one step
		    target = curbuf->b_u_save_nr_cur + step + 1;
		if (target <= 0)
		    // Go to before first write: before the oldest change. Use
		    // the sequence number for that.
		    dofile = FALSE;
	    }
	    else
	    {
		// Moving forward to a newer write.
		target = curbuf->b_u_save_nr_cur + step;
		if (target > curbuf->b_u_save_nr_last)
		{
		    // Go to after last write: after the latest change. Use
		    // the sequence number for that.
		    target = curbuf->b_u_seq_last + 1;
		    dofile = FALSE;
		}
	    }
	}
	else
	    target = curbuf->b_u_seq_cur + step;
	if (step < 0)
	{
	    if (target < 0)
		target = 0;
	    closest = -1;
	}
	else
	{
	    if (dosec)
		closest = (long)(vim_time() + 1);
	    else if (dofile)
		closest = curbuf->b_u_save_nr_last + 2;
	    else
		closest = curbuf->b_u_seq_last + 2;
	    if (target >= closest)
		target = closest - 1;
	}
    }
    closest_start = closest;
    closest_seq = curbuf->b_u_seq_cur;

    // When "target" is 0; Back to origin.
    if (target == 0)
    {
	mark = lastmark;  // avoid that GCC complains
	goto target_zero;
    }

    /*
     * May do this twice:
     * 1. Search for "target", update "closest" to the best match found.
     * 2. If "target" not found search for "closest".
     *
     * When using the closest time we use the sequence number in the second
     * round, because there may be several entries with the same time.
     */
    for (round = 1; round <= 2; ++round)
    {
	// Find the path from the current state to where we want to go.  The
	// desired state can be anywhere in the undo tree, need to go all over
	// it.  We put "nomark" in uh_walk where we have been without success,
	// "mark" where it could possibly be.
	mark = ++lastmark;
	nomark = ++lastmark;

	if (curbuf->b_u_curhead == NULL)	// at leaf of the tree
	    uhp = curbuf->b_u_newhead;
	else
	    uhp = curbuf->b_u_curhead;

	while (uhp != NULL)
	{
	    uhp->uh_walk = mark;
	    if (dosec)
		val = (long)(uhp->uh_time);
	    else if (dofile)
		val = uhp->uh_save_nr;
	    else
		val = uhp->uh_seq;

	    if (round == 1 && !(dofile && val == 0))
	    {
		// Remember the header that is closest to the target.
		// It must be at least in the right direction (checked with
		// "b_u_seq_cur").  When the timestamp is equal find the
		// highest/lowest sequence number.
		if ((step < 0 ? uhp->uh_seq <= curbuf->b_u_seq_cur
			      : uhp->uh_seq > curbuf->b_u_seq_cur)
			&& ((dosec && val == closest)
			    ? (step < 0
				? uhp->uh_seq < closest_seq
				: uhp->uh_seq > closest_seq)
			    : closest == closest_start
				|| (val > target
				    ? (closest > target
					? val - target <= closest - target
					: val - target <= target - closest)
				    : (closest > target
					? target - val <= closest - target
					: target - val <= target - closest))))
		{
		    closest = val;
		    closest_seq = uhp->uh_seq;
		}
	    }

	    // Quit searching when we found a match.  But when searching for a
	    // time we need to continue looking for the best uh_seq.
	    if (target == val && !dosec)
	    {
		target = uhp->uh_seq;
		break;
	    }

	    // go down in the tree if we haven't been there
	    if (uhp->uh_prev.ptr != NULL && uhp->uh_prev.ptr->uh_walk != nomark
					 && uhp->uh_prev.ptr->uh_walk != mark)
		uhp = uhp->uh_prev.ptr;

	    // go to alternate branch if we haven't been there
	    else if (uhp->uh_alt_next.ptr != NULL
		    && uhp->uh_alt_next.ptr->uh_walk != nomark
		    && uhp->uh_alt_next.ptr->uh_walk != mark)
		uhp = uhp->uh_alt_next.ptr;

	    // go up in the tree if we haven't been there and we are at the
	    // start of alternate branches
	    else if (uhp->uh_next.ptr != NULL && uhp->uh_alt_prev.ptr == NULL
		    && uhp->uh_next.ptr->uh_walk != nomark
		    && uhp->uh_next.ptr->uh_walk != mark)
	    {
		// If still at the start we don't go through this change.
		if (uhp == curbuf->b_u_curhead)
		    uhp->uh_walk = nomark;
		uhp = uhp->uh_next.ptr;
	    }

	    else
	    {
		// need to backtrack; mark this node as useless
		uhp->uh_walk = nomark;
		if (uhp->uh_alt_prev.ptr != NULL)
		    uhp = uhp->uh_alt_prev.ptr;
		else
		    uhp = uhp->uh_next.ptr;
	    }
	}

	if (uhp != NULL)    // found it
	    break;

	if (absolute)
	{
	    semsg(_(e_undo_number_nr_not_found), step);
	    return;
	}

	if (closest == closest_start)
	{
	    if (step < 0)
		msg(_("Already at oldest change"));
	    else
		msg(_("Already at newest change"));
	    return;
	}

	target = closest_seq;
	dosec = FALSE;
	dofile = FALSE;
	if (step < 0)
	    above = TRUE;	// stop above the header
    }

target_zero:
    // If we found it: Follow the path to go to where we want to be.
    if (uhp != NULL || target == 0)
    {
	/*
	 * First go up the tree as much as needed.
	 */
	while (!got_int)
	{
	    // Do the change warning now, for the same reason as above.
	    change_warning(0);

	    uhp = curbuf->b_u_curhead;
	    if (uhp == NULL)
		uhp = curbuf->b_u_newhead;
	    else
		uhp = uhp->uh_next.ptr;
	    if (uhp == NULL || (target > 0 && uhp->uh_walk != mark)
					 || (uhp->uh_seq == target && !above))
		break;
	    curbuf->b_u_curhead = uhp;
	    u_undoredo(TRUE);
	    if (target > 0)
		uhp->uh_walk = nomark;	// don't go back down here
	}

	// When back to origin, redo is not needed.
	if (target > 0)
	{
	    /*
	     * And now go down the tree (redo), branching off where needed.
	     */
	    while (!got_int)
	    {
		// Do the change warning now, for the same reason as above.
		change_warning(0);

		uhp = curbuf->b_u_curhead;
		if (uhp == NULL)
		    break;

		// Go back to the first branch with a mark.
		while (uhp->uh_alt_prev.ptr != NULL
				     && uhp->uh_alt_prev.ptr->uh_walk == mark)
		    uhp = uhp->uh_alt_prev.ptr;

		// Find the last branch with a mark, that's the one.
		last = uhp;
		while (last->uh_alt_next.ptr != NULL
				    && last->uh_alt_next.ptr->uh_walk == mark)
		    last = last->uh_alt_next.ptr;
		if (last != uhp)
		{
		    // Make the used branch the first entry in the list of
		    // alternatives to make "u" and CTRL-R take this branch.
		    while (uhp->uh_alt_prev.ptr != NULL)
			uhp = uhp->uh_alt_prev.ptr;
		    if (last->uh_alt_next.ptr != NULL)
			last->uh_alt_next.ptr->uh_alt_prev.ptr =
							last->uh_alt_prev.ptr;
		    last->uh_alt_prev.ptr->uh_alt_next.ptr =
							last->uh_alt_next.ptr;
		    last->uh_alt_prev.ptr = NULL;
		    last->uh_alt_next.ptr = uhp;
		    uhp->uh_alt_prev.ptr = last;

		    if (curbuf->b_u_oldhead == uhp)
			curbuf->b_u_oldhead = last;
		    uhp = last;
		    if (uhp->uh_next.ptr != NULL)
			uhp->uh_next.ptr->uh_prev.ptr = uhp;
		}
		curbuf->b_u_curhead = uhp;

		if (uhp->uh_walk != mark)
		    break;	    // must have reached the target

		// Stop when going backwards in time and didn't find the exact
		// header we were looking for.
		if (uhp->uh_seq == target && above)
		{
		    curbuf->b_u_seq_cur = target - 1;
		    break;
		}

		u_undoredo(FALSE);

		// Advance "curhead" to below the header we last used.  If it
		// becomes NULL then we need to set "newhead" to this leaf.
		if (uhp->uh_prev.ptr == NULL)
		    curbuf->b_u_newhead = uhp;
		curbuf->b_u_curhead = uhp->uh_prev.ptr;
		did_undo = FALSE;

		if (uhp->uh_seq == target)	// found it!
		    break;

		uhp = uhp->uh_prev.ptr;
		if (uhp == NULL || uhp->uh_walk != mark)
		{
		    // Need to redo more but can't find it...
		    internal_error("undo_time()");
		    break;
		}
	    }
	}
    }
    u_undo_end(did_undo, absolute);
}