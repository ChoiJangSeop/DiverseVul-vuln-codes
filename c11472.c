changed_common(
    linenr_T	lnum,
    colnr_T	col,
    linenr_T	lnume,
    long	xtra)
{
    win_T	*wp;
    tabpage_T	*tp;
    int		i;
    int		cols;
    pos_T	*p;
    int		add;

    // mark the buffer as modified
    changed();

#ifdef FEAT_EVAL
    may_record_change(lnum, col, lnume, xtra);
#endif
#ifdef FEAT_DIFF
    if (curwin->w_p_diff && diff_internal())
	curtab->tp_diff_update = TRUE;
#endif

    // set the '. mark
    if ((cmdmod.cmod_flags & CMOD_KEEPJUMPS) == 0)
    {
	curbuf->b_last_change.lnum = lnum;
	curbuf->b_last_change.col = col;

	// Create a new entry if a new undo-able change was started or we
	// don't have an entry yet.
	if (curbuf->b_new_change || curbuf->b_changelistlen == 0)
	{
	    if (curbuf->b_changelistlen == 0)
		add = TRUE;
	    else
	    {
		// Don't create a new entry when the line number is the same
		// as the last one and the column is not too far away.  Avoids
		// creating many entries for typing "xxxxx".
		p = &curbuf->b_changelist[curbuf->b_changelistlen - 1];
		if (p->lnum != lnum)
		    add = TRUE;
		else
		{
		    cols = comp_textwidth(FALSE);
		    if (cols == 0)
			cols = 79;
		    add = (p->col + cols < col || col + cols < p->col);
		}
	    }
	    if (add)
	    {
		// This is the first of a new sequence of undo-able changes
		// and it's at some distance of the last change.  Use a new
		// position in the changelist.
		curbuf->b_new_change = FALSE;

		if (curbuf->b_changelistlen == JUMPLISTSIZE)
		{
		    // changelist is full: remove oldest entry
		    curbuf->b_changelistlen = JUMPLISTSIZE - 1;
		    mch_memmove(curbuf->b_changelist, curbuf->b_changelist + 1,
					  sizeof(pos_T) * (JUMPLISTSIZE - 1));
		    FOR_ALL_TAB_WINDOWS(tp, wp)
		    {
			// Correct position in changelist for other windows on
			// this buffer.
			if (wp->w_buffer == curbuf && wp->w_changelistidx > 0)
			    --wp->w_changelistidx;
		    }
		}
		FOR_ALL_TAB_WINDOWS(tp, wp)
		{
		    // For other windows, if the position in the changelist is
		    // at the end it stays at the end.
		    if (wp->w_buffer == curbuf
			    && wp->w_changelistidx == curbuf->b_changelistlen)
			++wp->w_changelistidx;
		}
		++curbuf->b_changelistlen;
	    }
	}
	curbuf->b_changelist[curbuf->b_changelistlen - 1] =
							curbuf->b_last_change;
	// The current window is always after the last change, so that "g,"
	// takes you back to it.
	curwin->w_changelistidx = curbuf->b_changelistlen;
    }

    FOR_ALL_TAB_WINDOWS(tp, wp)
    {
	if (wp->w_buffer == curbuf)
	{
#ifdef FEAT_FOLDING
	    linenr_T last = lnume + xtra - 1;  // last line after the change
#endif
	    // Mark this window to be redrawn later.
	    if (wp->w_redr_type < VALID)
		wp->w_redr_type = VALID;

	    // Check if a change in the buffer has invalidated the cached
	    // values for the cursor.
#ifdef FEAT_FOLDING
	    // Update the folds for this window.  Can't postpone this, because
	    // a following operator might work on the whole fold: ">>dd".
	    foldUpdate(wp, lnum, last);

	    // The change may cause lines above or below the change to become
	    // included in a fold.  Set lnum/lnume to the first/last line that
	    // might be displayed differently.
	    // Set w_cline_folded here as an efficient way to update it when
	    // inserting lines just above a closed fold.
	    i = hasFoldingWin(wp, lnum, &lnum, NULL, FALSE, NULL);
	    if (wp->w_cursor.lnum == lnum)
		wp->w_cline_folded = i;
	    i = hasFoldingWin(wp, last, NULL, &last, FALSE, NULL);
	    if (wp->w_cursor.lnum == last)
		wp->w_cline_folded = i;

	    // If the changed line is in a range of previously folded lines,
	    // compare with the first line in that range.
	    if (wp->w_cursor.lnum <= lnum)
	    {
		i = find_wl_entry(wp, lnum);
		if (i >= 0 && wp->w_cursor.lnum > wp->w_lines[i].wl_lnum)
		    changed_line_abv_curs_win(wp);
	    }
#endif
	    if (wp->w_cursor.lnum > lnum)
		changed_line_abv_curs_win(wp);
	    else if (wp->w_cursor.lnum == lnum && wp->w_cursor.col >= col)
		changed_cline_bef_curs_win(wp);
	    if (wp->w_botline >= lnum)
	    {
		if (xtra < 0)
		    invalidate_botline_win(wp);
		else
		    // Assume that botline doesn't change (inserted lines make
		    // other lines scroll down below botline).
		    approximate_botline_win(wp);
	    }

	    // Check if any w_lines[] entries have become invalid.
	    // For entries below the change: Correct the lnums for
	    // inserted/deleted lines.  Makes it possible to stop displaying
	    // after the change.
	    for (i = 0; i < wp->w_lines_valid; ++i)
		if (wp->w_lines[i].wl_valid)
		{
		    if (wp->w_lines[i].wl_lnum >= lnum)
		    {
			if (wp->w_lines[i].wl_lnum < lnume)
			{
			    // line included in change
			    wp->w_lines[i].wl_valid = FALSE;
			}
			else if (xtra != 0)
			{
			    // line below change
			    wp->w_lines[i].wl_lnum += xtra;
#ifdef FEAT_FOLDING
			    wp->w_lines[i].wl_lastlnum += xtra;
#endif
			}
		    }
#ifdef FEAT_FOLDING
		    else if (wp->w_lines[i].wl_lastlnum >= lnum)
		    {
			// change somewhere inside this range of folded lines,
			// may need to be redrawn
			wp->w_lines[i].wl_valid = FALSE;
		    }
#endif
		}

#ifdef FEAT_FOLDING
	    // Take care of side effects for setting w_topline when folds have
	    // changed.  Esp. when the buffer was changed in another window.
	    if (hasAnyFolding(wp))
		set_topline(wp, wp->w_topline);
#endif
	    // If lines have been added or removed, relative numbering always
	    // requires a redraw.
	    if (wp->w_p_rnu && xtra != 0)
	    {
		wp->w_last_cursor_lnum_rnu = 0;
		redraw_win_later(wp, VALID);
	    }
#ifdef FEAT_SYN_HL
	    // Cursor line highlighting probably need to be updated with
	    // "VALID" if it's below the change.
	    // If the cursor line is inside the change we need to redraw more.
	    if (wp->w_p_cul)
	    {
		if (xtra == 0)
		    redraw_win_later(wp, VALID);
		else if (lnum <= wp->w_last_cursorline)
		    redraw_win_later(wp, SOME_VALID);
	    }
#endif
	}
    }

    // Call update_screen() later, which checks out what needs to be redrawn,
    // since it notices b_mod_set and then uses b_mod_*.
    if (must_redraw < VALID)
	must_redraw = VALID;

    // when the cursor line is changed always trigger CursorMoved
    if (lnum <= curwin->w_cursor.lnum
		 && lnume + (xtra < 0 ? -xtra : xtra) > curwin->w_cursor.lnum)
	last_cursormoved.lnum = 0;
}