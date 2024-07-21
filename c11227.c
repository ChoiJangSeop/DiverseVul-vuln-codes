do_buffer_ext(
    int		action,
    int		start,
    int		dir,		// FORWARD or BACKWARD
    int		count,		// buffer number or number of buffers
    int		flags)		// DOBUF_FORCEIT etc.
{
    buf_T	*buf;
    buf_T	*bp;
    int		unload = (action == DOBUF_UNLOAD || action == DOBUF_DEL
			|| action == DOBUF_WIPE || action == DOBUF_WIPE_REUSE);

    switch (start)
    {
	case DOBUF_FIRST:   buf = firstbuf; break;
	case DOBUF_LAST:    buf = lastbuf;  break;
	default:	    buf = curbuf;   break;
    }
    if (start == DOBUF_MOD)	    // find next modified buffer
    {
	while (count-- > 0)
	{
	    do
	    {
		buf = buf->b_next;
		if (buf == NULL)
		    buf = firstbuf;
	    }
	    while (buf != curbuf && !bufIsChanged(buf));
	}
	if (!bufIsChanged(buf))
	{
	    emsg(_(e_no_modified_buffer_found));
	    return FAIL;
	}
    }
    else if (start == DOBUF_FIRST && count) // find specified buffer number
    {
	while (buf != NULL && buf->b_fnum != count)
	    buf = buf->b_next;
    }
    else
    {
	bp = NULL;
	while (count > 0 || (!unload && !buf->b_p_bl && bp != buf))
	{
	    // remember the buffer where we start, we come back there when all
	    // buffers are unlisted.
	    if (bp == NULL)
		bp = buf;
	    if (dir == FORWARD)
	    {
		buf = buf->b_next;
		if (buf == NULL)
		    buf = firstbuf;
	    }
	    else
	    {
		buf = buf->b_prev;
		if (buf == NULL)
		    buf = lastbuf;
	    }
	    // don't count unlisted buffers
	    if (unload || buf->b_p_bl)
	    {
		 --count;
		 bp = NULL;	// use this buffer as new starting point
	    }
	    if (bp == buf)
	    {
		// back where we started, didn't find anything.
		emsg(_(e_there_is_no_listed_buffer));
		return FAIL;
	    }
	}
    }

    if (buf == NULL)	    // could not find it
    {
	if (start == DOBUF_FIRST)
	{
	    // don't warn when deleting
	    if (!unload)
		semsg(_(e_buffer_nr_does_not_exist), count);
	}
	else if (dir == FORWARD)
	    emsg(_(e_cannot_go_beyond_last_buffer));
	else
	    emsg(_(e_cannot_go_before_first_buffer));
	return FAIL;
    }
#ifdef FEAT_PROP_POPUP
    if ((flags & DOBUF_NOPOPUP) && bt_popup(buf)
# ifdef FEAT_TERMINAL
				&& !bt_terminal(buf)
#endif
       )
	return OK;
#endif

#ifdef FEAT_GUI
    need_mouse_correct = TRUE;
#endif

    /*
     * delete buffer "buf" from memory and/or the list
     */
    if (unload)
    {
	int	forward;
	bufref_T bufref;

	if (!can_unload_buffer(buf))
	    return FAIL;

	set_bufref(&bufref, buf);

	// When unloading or deleting a buffer that's already unloaded and
	// unlisted: fail silently.
	if (action != DOBUF_WIPE && action != DOBUF_WIPE_REUSE
				   && buf->b_ml.ml_mfp == NULL && !buf->b_p_bl)
	    return FAIL;

	if ((flags & DOBUF_FORCEIT) == 0 && bufIsChanged(buf))
	{
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	    if ((p_confirm || (cmdmod.cmod_flags & CMOD_CONFIRM)) && p_write)
	    {
		dialog_changed(buf, FALSE);
		if (!bufref_valid(&bufref))
		    // Autocommand deleted buffer, oops!  It's not changed
		    // now.
		    return FAIL;
		// If it's still changed fail silently, the dialog already
		// mentioned why it fails.
		if (bufIsChanged(buf))
		    return FAIL;
	    }
	    else
#endif
	    {
		semsg(_(e_no_write_since_last_change_for_buffer_nr_add_bang_to_override),
								 buf->b_fnum);
		return FAIL;
	    }
	}

	// When closing the current buffer stop Visual mode.
	if (buf == curbuf && VIsual_active)
	    end_visual_mode();

	// If deleting the last (listed) buffer, make it empty.
	// The last (listed) buffer cannot be unloaded.
	FOR_ALL_BUFFERS(bp)
	    if (bp->b_p_bl && bp != buf)
		break;
	if (bp == NULL && buf == curbuf)
	    return empty_curbuf(TRUE, (flags & DOBUF_FORCEIT), action);

	// If the deleted buffer is the current one, close the current window
	// (unless it's the only window).  Repeat this so long as we end up in
	// a window with this buffer.
	while (buf == curbuf
		   && !(curwin->w_closing || curwin->w_buffer->b_locked > 0)
		   && (!ONE_WINDOW || first_tabpage->tp_next != NULL))
	{
	    if (win_close(curwin, FALSE) == FAIL)
		break;
	}

	// If the buffer to be deleted is not the current one, delete it here.
	if (buf != curbuf)
	{
	    close_windows(buf, FALSE);
	    if (buf != curbuf && bufref_valid(&bufref) && buf->b_nwindows <= 0)
		    close_buffer(NULL, buf, action, FALSE, FALSE);
	    return OK;
	}

	/*
	 * Deleting the current buffer: Need to find another buffer to go to.
	 * There should be another, otherwise it would have been handled
	 * above.  However, autocommands may have deleted all buffers.
	 * First use au_new_curbuf.br_buf, if it is valid.
	 * Then prefer the buffer we most recently visited.
	 * Else try to find one that is loaded, after the current buffer,
	 * then before the current buffer.
	 * Finally use any buffer.
	 */
	buf = NULL;	// selected buffer
	bp = NULL;	// used when no loaded buffer found
	if (au_new_curbuf.br_buf != NULL && bufref_valid(&au_new_curbuf))
	    buf = au_new_curbuf.br_buf;
	else if (curwin->w_jumplistlen > 0)
	{
	    int     jumpidx;

	    jumpidx = curwin->w_jumplistidx - 1;
	    if (jumpidx < 0)
		jumpidx = curwin->w_jumplistlen - 1;

	    forward = jumpidx;
	    while (jumpidx != curwin->w_jumplistidx)
	    {
		buf = buflist_findnr(curwin->w_jumplist[jumpidx].fmark.fnum);
		if (buf != NULL)
		{
		    if (buf == curbuf || !buf->b_p_bl)
			buf = NULL;	// skip current and unlisted bufs
		    else if (buf->b_ml.ml_mfp == NULL)
		    {
			// skip unloaded buf, but may keep it for later
			if (bp == NULL)
			    bp = buf;
			buf = NULL;
		    }
		}
		if (buf != NULL)   // found a valid buffer: stop searching
		    break;
		// advance to older entry in jump list
		if (!jumpidx && curwin->w_jumplistidx == curwin->w_jumplistlen)
		    break;
		if (--jumpidx < 0)
		    jumpidx = curwin->w_jumplistlen - 1;
		if (jumpidx == forward)		// List exhausted for sure
		    break;
	    }
	}

	if (buf == NULL)	// No previous buffer, Try 2'nd approach
	{
	    forward = TRUE;
	    buf = curbuf->b_next;
	    for (;;)
	    {
		if (buf == NULL)
		{
		    if (!forward)	// tried both directions
			break;
		    buf = curbuf->b_prev;
		    forward = FALSE;
		    continue;
		}
		// in non-help buffer, try to skip help buffers, and vv
		if (buf->b_help == curbuf->b_help && buf->b_p_bl)
		{
		    if (buf->b_ml.ml_mfp != NULL)   // found loaded buffer
			break;
		    if (bp == NULL)	// remember unloaded buf for later
			bp = buf;
		}
		if (forward)
		    buf = buf->b_next;
		else
		    buf = buf->b_prev;
	    }
	}
	if (buf == NULL)	// No loaded buffer, use unloaded one
	    buf = bp;
	if (buf == NULL)	// No loaded buffer, find listed one
	{
	    FOR_ALL_BUFFERS(buf)
		if (buf->b_p_bl && buf != curbuf)
		    break;
	}
	if (buf == NULL)	// Still no buffer, just take one
	{
	    if (curbuf->b_next != NULL)
		buf = curbuf->b_next;
	    else
		buf = curbuf->b_prev;
	}
    }

    if (buf == NULL)
    {
	// Autocommands must have wiped out all other buffers.  Only option
	// now is to make the current buffer empty.
	return empty_curbuf(FALSE, (flags & DOBUF_FORCEIT), action);
    }

    /*
     * make "buf" the current buffer
     */
    if (action == DOBUF_SPLIT)	    // split window first
    {
	// If 'switchbuf' contains "useopen": jump to first window containing
	// "buf" if one exists
	if ((swb_flags & SWB_USEOPEN) && buf_jump_open_win(buf))
	    return OK;
	// If 'switchbuf' contains "usetab": jump to first window in any tab
	// page containing "buf" if one exists
	if ((swb_flags & SWB_USETAB) && buf_jump_open_tab(buf))
	    return OK;
	if (win_split(0, 0) == FAIL)
	    return FAIL;
    }

    // go to current buffer - nothing to do
    if (buf == curbuf)
	return OK;

    // Check if the current buffer may be abandoned.
    if (action == DOBUF_GOTO && !can_abandon(curbuf, (flags & DOBUF_FORCEIT)))
    {
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	if ((p_confirm || (cmdmod.cmod_flags & CMOD_CONFIRM)) && p_write)
	{
	    bufref_T bufref;

	    set_bufref(&bufref, buf);
	    dialog_changed(curbuf, FALSE);
	    if (!bufref_valid(&bufref))
		// Autocommand deleted buffer, oops!
		return FAIL;
	}
	if (bufIsChanged(curbuf))
#endif
	{
	    no_write_message();
	    return FAIL;
	}
    }

    // Go to the other buffer.
    set_curbuf(buf, action);

    if (action == DOBUF_SPLIT)
	RESET_BINDING(curwin);	// reset 'scrollbind' and 'cursorbind'

#if defined(FEAT_EVAL)
    if (aborting())	    // autocmds may abort script processing
	return FAIL;
#endif

    return OK;
}