ins_compl_next(
    int	    allow_get_expansion,
    int	    count,		// repeat completion this many times; should
				// be at least 1
    int	    insert_match,	// Insert the newly selected match
    int	    in_compl_func)	// called from complete_check()
{
    int	    num_matches = -1;
    int	    todo = count;
    int	    advance;
    int	    started = compl_started;

    // When user complete function return -1 for findstart which is next
    // time of 'always', compl_shown_match become NULL.
    if (compl_shown_match == NULL)
	return -1;

    if (compl_leader != NULL && !match_at_original_text(compl_shown_match))
	// Update "compl_shown_match" to the actually shown match
	ins_compl_update_shown_match();

    if (allow_get_expansion && insert_match
	    && (!(compl_get_longest || compl_restarting) || compl_used_match))
	// Delete old text to be replaced
	ins_compl_delete();

    // When finding the longest common text we stick at the original text,
    // don't let CTRL-N or CTRL-P move to the first match.
    advance = count != 1 || !allow_get_expansion || !compl_get_longest;

    // When restarting the search don't insert the first match either.
    if (compl_restarting)
    {
	advance = FALSE;
	compl_restarting = FALSE;
    }

    // Repeat this for when <PageUp> or <PageDown> is typed.  But don't wrap
    // around.
    if (find_next_completion_match(allow_get_expansion, todo, advance,
							&num_matches) == -1)
	return -1;

    // Insert the text of the new completion, or the compl_leader.
    if (compl_no_insert && !started)
    {
	ins_bytes(compl_orig_text + get_compl_len());
	compl_used_match = FALSE;
    }
    else if (insert_match)
    {
	if (!compl_get_longest || compl_used_match)
	    ins_compl_insert(in_compl_func);
	else
	    ins_bytes(compl_leader + get_compl_len());
    }
    else
	compl_used_match = FALSE;

    if (!allow_get_expansion)
    {
	// may undisplay the popup menu first
	ins_compl_upd_pum();

	if (pum_enough_matches())
	    // Will display the popup menu, don't redraw yet to avoid flicker.
	    pum_call_update_screen();
	else
	    // Not showing the popup menu yet, redraw to show the user what was
	    // inserted.
	    update_screen(0);

	// display the updated popup menu
	ins_compl_show_pum();
#ifdef FEAT_GUI
	if (gui.in_use)
	{
	    // Show the cursor after the match, not after the redrawn text.
	    setcursor();
	    out_flush_cursor(FALSE, FALSE);
	}
#endif

	// Delete old text to be replaced, since we're still searching and
	// don't want to match ourselves!
	ins_compl_delete();
    }

    // Enter will select a match when the match wasn't inserted and the popup
    // menu is visible.
    if (compl_no_insert && !started)
	compl_enter_selects = TRUE;
    else
	compl_enter_selects = !insert_match && compl_match_array != NULL;

    // Show the file name for the match (if any)
    if (compl_shown_match->cp_fname != NULL)
	ins_compl_show_filename();

    return num_matches;
}