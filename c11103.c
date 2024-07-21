set_curbuf(buf_T *buf, int action)
{
    buf_T	*prevbuf;
    int		unload = (action == DOBUF_UNLOAD || action == DOBUF_DEL
			|| action == DOBUF_WIPE || action == DOBUF_WIPE_REUSE);
#ifdef FEAT_SYN_HL
    long	old_tw = curbuf->b_p_tw;
#endif
    bufref_T	newbufref;
    bufref_T	prevbufref;

    setpcmark();
    if ((cmdmod.cmod_flags & CMOD_KEEPALT) == 0)
	curwin->w_alt_fnum = curbuf->b_fnum; // remember alternate file
    buflist_altfpos(curwin);			 // remember curpos

    // Don't restart Select mode after switching to another buffer.
    VIsual_reselect = FALSE;

    // close_windows() or apply_autocmds() may change curbuf and wipe out "buf"
    prevbuf = curbuf;
    set_bufref(&prevbufref, prevbuf);
    set_bufref(&newbufref, buf);

    // Autocommands may delete the current buffer and/or the buffer we want to
    // go to.  In those cases don't close the buffer.
    if (!apply_autocmds(EVENT_BUFLEAVE, NULL, NULL, FALSE, curbuf)
	    || (bufref_valid(&prevbufref)
		&& bufref_valid(&newbufref)
#ifdef FEAT_EVAL
		&& !aborting()
#endif
	       ))
    {
#ifdef FEAT_SYN_HL
	if (prevbuf == curwin->w_buffer)
	    reset_synblock(curwin);
#endif
	if (unload)
	    close_windows(prevbuf, FALSE);
#if defined(FEAT_EVAL)
	if (bufref_valid(&prevbufref) && !aborting())
#else
	if (bufref_valid(&prevbufref))
#endif
	{
	    win_T  *previouswin = curwin;

	    // Do not sync when in Insert mode and the buffer is open in
	    // another window, might be a timer doing something in another
	    // window.
	    if (prevbuf == curbuf
			 && ((State & INSERT) == 0 || curbuf->b_nwindows <= 1))
		u_sync(FALSE);
	    close_buffer(prevbuf == curwin->w_buffer ? curwin : NULL, prevbuf,
		    unload ? action : (action == DOBUF_GOTO
			&& !buf_hide(prevbuf)
			&& !bufIsChanged(prevbuf)) ? DOBUF_UNLOAD : 0,
		    FALSE, FALSE);
	    if (curwin != previouswin && win_valid(previouswin))
	      // autocommands changed curwin, Grr!
	      curwin = previouswin;
	}
    }
    // An autocommand may have deleted "buf", already entered it (e.g., when
    // it did ":bunload") or aborted the script processing.
    // If curwin->w_buffer is null, enter_buffer() will make it valid again
    if ((buf_valid(buf) && buf != curbuf
#ifdef FEAT_EVAL
		&& !aborting()
#endif
	) || curwin->w_buffer == NULL)
    {
	enter_buffer(buf);
#ifdef FEAT_SYN_HL
	if (old_tw != curbuf->b_p_tw)
	    check_colorcolumn(curwin);
#endif
    }
}