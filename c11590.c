ex_diffgetput(exarg_T *eap)
{
    linenr_T	lnum;
    int		count;
    linenr_T	off = 0;
    diff_T	*dp;
    diff_T	*dprev;
    diff_T	*dfree;
    int		idx_cur;
    int		idx_other;
    int		idx_from;
    int		idx_to;
    int		i;
    int		added;
    char_u	*p;
    aco_save_T	aco;
    buf_T	*buf;
    int		start_skip, end_skip;
    int		new_count;
    int		buf_empty;
    int		found_not_ma = FALSE;

    // Find the current buffer in the list of diff buffers.
    idx_cur = diff_buf_idx(curbuf);
    if (idx_cur == DB_COUNT)
    {
	emsg(_(e_current_buffer_is_not_in_diff_mode));
	return;
    }

    if (*eap->arg == NUL)
    {
	// No argument: Find the other buffer in the list of diff buffers.
	for (idx_other = 0; idx_other < DB_COUNT; ++idx_other)
	    if (curtab->tp_diffbuf[idx_other] != curbuf
		    && curtab->tp_diffbuf[idx_other] != NULL)
	    {
		if (eap->cmdidx != CMD_diffput
				     || curtab->tp_diffbuf[idx_other]->b_p_ma)
		    break;
		found_not_ma = TRUE;
	    }
	if (idx_other == DB_COUNT)
	{
	    if (found_not_ma)
		emsg(_(e_no_other_buffer_in_diff_mode_is_modifiable));
	    else
		emsg(_(e_no_other_buffer_in_diff_mode));
	    return;
	}

	// Check that there isn't a third buffer in the list
	for (i = idx_other + 1; i < DB_COUNT; ++i)
	    if (curtab->tp_diffbuf[i] != curbuf
		    && curtab->tp_diffbuf[i] != NULL
		    && (eap->cmdidx != CMD_diffput || curtab->tp_diffbuf[i]->b_p_ma))
	    {
		emsg(_(e_more_than_two_buffers_in_diff_mode_dont_know_which_one_to_use));
		return;
	    }
    }
    else
    {
	// Buffer number or pattern given.  Ignore trailing white space.
	p = eap->arg + STRLEN(eap->arg);
	while (p > eap->arg && VIM_ISWHITE(p[-1]))
	    --p;
	for (i = 0; vim_isdigit(eap->arg[i]) && eap->arg + i < p; ++i)
	    ;
	if (eap->arg + i == p)	    // digits only
	    i = atol((char *)eap->arg);
	else
	{
	    i = buflist_findpat(eap->arg, p, FALSE, TRUE, FALSE);
	    if (i < 0)
		return;		// error message already given
	}
	buf = buflist_findnr(i);
	if (buf == NULL)
	{
	    semsg(_(e_cant_find_buffer_str), eap->arg);
	    return;
	}
	if (buf == curbuf)
	    return;		// nothing to do
	idx_other = diff_buf_idx(buf);
	if (idx_other == DB_COUNT)
	{
	    semsg(_(e_buffer_str_is_not_in_diff_mode), eap->arg);
	    return;
	}
    }

    diff_busy = TRUE;

    // When no range given include the line above or below the cursor.
    if (eap->addr_count == 0)
    {
	// Make it possible that ":diffget" on the last line gets line below
	// the cursor line when there is no difference above the cursor.
	if (eap->cmdidx == CMD_diffget
		&& eap->line1 == curbuf->b_ml.ml_line_count
		&& diff_check(curwin, eap->line1) == 0
		&& (eap->line1 == 1 || diff_check(curwin, eap->line1 - 1) == 0))
	    ++eap->line2;
	else if (eap->line1 > 0)
	    --eap->line1;
    }

    if (eap->cmdidx == CMD_diffget)
    {
	idx_from = idx_other;
	idx_to = idx_cur;
    }
    else
    {
	idx_from = idx_cur;
	idx_to = idx_other;
	// Need to make the other buffer the current buffer to be able to make
	// changes in it.
	// set curwin/curbuf to buf and save a few things
	aucmd_prepbuf(&aco, curtab->tp_diffbuf[idx_other]);
    }

    // May give the warning for a changed buffer here, which can trigger the
    // FileChangedRO autocommand, which may do nasty things and mess
    // everything up.
    if (!curbuf->b_changed)
    {
	change_warning(0);
	if (diff_buf_idx(curbuf) != idx_to)
	{
	    emsg(_(e_buffer_changed_unexpectedly));
	    goto theend;
	}
    }

    dprev = NULL;
    for (dp = curtab->tp_first_diff; dp != NULL; )
    {
	if (dp->df_lnum[idx_cur] > eap->line2 + off)
	    break;	// past the range that was specified

	dfree = NULL;
	lnum = dp->df_lnum[idx_to];
	count = dp->df_count[idx_to];
	if (dp->df_lnum[idx_cur] + dp->df_count[idx_cur] > eap->line1 + off
		&& u_save(lnum - 1, lnum + count) != FAIL)
	{
	    // Inside the specified range and saving for undo worked.
	    start_skip = 0;
	    end_skip = 0;
	    if (eap->addr_count > 0)
	    {
		// A range was specified: check if lines need to be skipped.
		start_skip = eap->line1 + off - dp->df_lnum[idx_cur];
		if (start_skip > 0)
		{
		    // range starts below start of current diff block
		    if (start_skip > count)
		    {
			lnum += count;
			count = 0;
		    }
		    else
		    {
			count -= start_skip;
			lnum += start_skip;
		    }
		}
		else
		    start_skip = 0;

		end_skip = dp->df_lnum[idx_cur] + dp->df_count[idx_cur] - 1
							 - (eap->line2 + off);
		if (end_skip > 0)
		{
		    // range ends above end of current/from diff block
		    if (idx_cur == idx_from)	// :diffput
		    {
			i = dp->df_count[idx_cur] - start_skip - end_skip;
			if (count > i)
			    count = i;
		    }
		    else			// :diffget
		    {
			count -= end_skip;
			end_skip = dp->df_count[idx_from] - start_skip - count;
			if (end_skip < 0)
			    end_skip = 0;
		    }
		}
		else
		    end_skip = 0;
	    }

	    buf_empty = BUFEMPTY();
	    added = 0;
	    for (i = 0; i < count; ++i)
	    {
		// remember deleting the last line of the buffer
		buf_empty = curbuf->b_ml.ml_line_count == 1;
		ml_delete(lnum);
		--added;
	    }
	    for (i = 0; i < dp->df_count[idx_from] - start_skip - end_skip; ++i)
	    {
		linenr_T nr;

		nr = dp->df_lnum[idx_from] + start_skip + i;
		if (nr > curtab->tp_diffbuf[idx_from]->b_ml.ml_line_count)
		    break;
		p = vim_strsave(ml_get_buf(curtab->tp_diffbuf[idx_from],
								  nr, FALSE));
		if (p != NULL)
		{
		    ml_append(lnum + i - 1, p, 0, FALSE);
		    vim_free(p);
		    ++added;
		    if (buf_empty && curbuf->b_ml.ml_line_count == 2)
		    {
			// Added the first line into an empty buffer, need to
			// delete the dummy empty line.
			buf_empty = FALSE;
			ml_delete((linenr_T)2);
		    }
		}
	    }
	    new_count = dp->df_count[idx_to] + added;
	    dp->df_count[idx_to] = new_count;

	    if (start_skip == 0 && end_skip == 0)
	    {
		// Check if there are any other buffers and if the diff is
		// equal in them.
		for (i = 0; i < DB_COUNT; ++i)
		    if (curtab->tp_diffbuf[i] != NULL && i != idx_from
								&& i != idx_to
			    && !diff_equal_entry(dp, idx_from, i))
			break;
		if (i == DB_COUNT)
		{
		    // delete the diff entry, the buffers are now equal here
		    dfree = dp;
		    dp = dp->df_next;
		    if (dprev == NULL)
			curtab->tp_first_diff = dp;
		    else
			dprev->df_next = dp;
		}
	    }

	    // Adjust marks.  This will change the following entries!
	    if (added != 0)
	    {
		mark_adjust(lnum, lnum + count - 1, (long)MAXLNUM, (long)added);
		if (curwin->w_cursor.lnum >= lnum)
		{
		    // Adjust the cursor position if it's in/after the changed
		    // lines.
		    if (curwin->w_cursor.lnum >= lnum + count)
			curwin->w_cursor.lnum += added;
		    else if (added < 0)
			curwin->w_cursor.lnum = lnum;
		}
	    }
	    changed_lines(lnum, 0, lnum + count, (long)added);

	    if (dfree != NULL)
	    {
		// Diff is deleted, update folds in other windows.
#ifdef FEAT_FOLDING
		diff_fold_update(dfree, idx_to);
#endif
		vim_free(dfree);
	    }
	    else
		// mark_adjust() may have changed the count in a wrong way
		dp->df_count[idx_to] = new_count;

	    // When changing the current buffer, keep track of line numbers
	    if (idx_cur == idx_to)
		off += added;
	}

	// If before the range or not deleted, go to next diff.
	if (dfree == NULL)
	{
	    dprev = dp;
	    dp = dp->df_next;
	}
    }

    // restore curwin/curbuf and a few other things
    if (eap->cmdidx != CMD_diffget)
    {
	// Syncing undo only works for the current buffer, but we change
	// another buffer.  Sync undo if the command was typed.  This isn't
	// 100% right when ":diffput" is used in a function or mapping.
	if (KeyTyped)
	    u_sync(FALSE);
	aucmd_restbuf(&aco);
    }

theend:
    diff_busy = FALSE;
    if (diff_need_update)
	ex_diffupdate(NULL);

    // Check that the cursor is on a valid character and update its
    // position.  When there were filler lines the topline has become
    // invalid.
    check_cursor();
    changed_line_abv_curs();

    if (diff_need_update)
	// redraw already done by ex_diffupdate()
	diff_need_update = FALSE;
    else
    {
	// Also need to redraw the other buffers.
	diff_redraw(FALSE);
	apply_autocmds(EVENT_DIFFUPDATED, NULL, NULL, FALSE, curbuf);
    }
}