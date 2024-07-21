ins_compl_stop(int c, int prev_mode, int retval)
{
    char_u	*ptr;
    int		want_cindent;

    // Get here when we have finished typing a sequence of ^N and
    // ^P or other completion characters in CTRL-X mode.  Free up
    // memory that was used, and make sure we can redo the insert.
    if (compl_curr_match != NULL || compl_leader != NULL || c == Ctrl_E)
    {
	// If any of the original typed text has been changed, eg when
	// ignorecase is set, we must add back-spaces to the redo
	// buffer.  We add as few as necessary to delete just the part
	// of the original text that has changed.
	// When using the longest match, edited the match or used
	// CTRL-E then don't use the current match.
	if (compl_curr_match != NULL && compl_used_match && c != Ctrl_E)
	    ptr = compl_curr_match->cp_str;
	else
	    ptr = NULL;
	ins_compl_fixRedoBufForLeader(ptr);
    }

    want_cindent = (get_can_cindent() && cindent_on());

    // When completing whole lines: fix indent for 'cindent'.
    // Otherwise, break line if it's too long.
    if (compl_cont_mode == CTRL_X_WHOLE_LINE)
    {
	// re-indent the current line
	if (want_cindent)
	{
	    do_c_expr_indent();
	    want_cindent = FALSE;	// don't do it again
	}
    }
    else
    {
	int prev_col = curwin->w_cursor.col;

	// put the cursor on the last char, for 'tw' formatting
	if (prev_col > 0)
	    dec_cursor();
	// only format when something was inserted
	if (!arrow_used && !ins_need_undo_get() && c != Ctrl_E)
	    insertchar(NUL, 0, -1);
	if (prev_col > 0
		&& ml_get_curline()[curwin->w_cursor.col] != NUL)
	    inc_cursor();
    }

    // If the popup menu is displayed pressing CTRL-Y means accepting
    // the selection without inserting anything.  When
    // compl_enter_selects is set the Enter key does the same.
    if ((c == Ctrl_Y || (compl_enter_selects
		    && (c == CAR || c == K_KENTER || c == NL)))
	    && pum_visible())
	retval = TRUE;

    // CTRL-E means completion is Ended, go back to the typed text.
    // but only do this, if the Popup is still visible
    if (c == Ctrl_E)
    {
	ins_compl_delete();
	if (compl_leader != NULL)
	    ins_bytes(compl_leader + get_compl_len());
	else if (compl_first_match != NULL)
	    ins_bytes(compl_orig_text + get_compl_len());
	retval = TRUE;
    }

    auto_format(FALSE, TRUE);

    // Trigger the CompleteDonePre event to give scripts a chance to
    // act upon the completion before clearing the info, and restore
    // ctrl_x_mode, so that complete_info() can be used.
    ctrl_x_mode = prev_mode;
    ins_apply_autocmds(EVENT_COMPLETEDONEPRE);

    ins_compl_free();
    compl_started = FALSE;
    compl_matches = 0;
    if (!shortmess(SHM_COMPLETIONMENU))
	msg_clr_cmdline();	// necessary for "noshowmode"
    ctrl_x_mode = CTRL_X_NORMAL;
    compl_enter_selects = FALSE;
    if (edit_submode != NULL)
    {
	edit_submode = NULL;
	showmode();
    }

#ifdef FEAT_CMDWIN
    if (c == Ctrl_C && cmdwin_type != 0)
	// Avoid the popup menu remains displayed when leaving the
	// command line window.
	update_screen(0);
#endif
    // Indent now if a key was typed that is in 'cinkeys'.
    if (want_cindent && in_cinkeys(KEY_COMPLETE, ' ', inindent(0)))
	do_c_expr_indent();
    // Trigger the CompleteDone event to give scripts a chance to act
    // upon the end of completion.
    ins_apply_autocmds(EVENT_COMPLETEDONE);

    return retval;
}