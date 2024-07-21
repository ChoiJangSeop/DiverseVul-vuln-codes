u_undo_end(
    int		did_undo,	// just did an undo
    int		absolute)	// used ":undo N"
{
    char	*msgstr;
    u_header_T	*uhp;
    char_u	msgbuf[80];

#ifdef FEAT_FOLDING
    if ((fdo_flags & FDO_UNDO) && KeyTyped)
	foldOpenCursor();
#endif

    if (global_busy	    // no messages now, wait until global is finished
	    || !messaging())  // 'lazyredraw' set, don't do messages now
	return;

    if (curbuf->b_ml.ml_flags & ML_EMPTY)
	--u_newcount;

    u_oldcount -= u_newcount;
    if (u_oldcount == -1)
	msgstr = N_("more line");
    else if (u_oldcount < 0)
	msgstr = N_("more lines");
    else if (u_oldcount == 1)
	msgstr = N_("line less");
    else if (u_oldcount > 1)
	msgstr = N_("fewer lines");
    else
    {
	u_oldcount = u_newcount;
	if (u_newcount == 1)
	    msgstr = N_("change");
	else
	    msgstr = N_("changes");
    }

    if (curbuf->b_u_curhead != NULL)
    {
	// For ":undo N" we prefer a "after #N" message.
	if (absolute && curbuf->b_u_curhead->uh_next.ptr != NULL)
	{
	    uhp = curbuf->b_u_curhead->uh_next.ptr;
	    did_undo = FALSE;
	}
	else if (did_undo)
	    uhp = curbuf->b_u_curhead;
	else
	    uhp = curbuf->b_u_curhead->uh_next.ptr;
    }
    else
	uhp = curbuf->b_u_newhead;

    if (uhp == NULL)
	*msgbuf = NUL;
    else
	add_time(msgbuf, sizeof(msgbuf), uhp->uh_time);

#ifdef FEAT_CONCEAL
    {
	win_T	*wp;

	FOR_ALL_WINDOWS(wp)
	{
	    if (wp->w_buffer == curbuf && wp->w_p_cole > 0)
		redraw_win_later(wp, NOT_VALID);
	}
    }
#endif

    smsg_attr_keep(0, _("%ld %s; %s #%ld  %s"),
	    u_oldcount < 0 ? -u_oldcount : u_oldcount,
	    _(msgstr),
	    did_undo ? _("before") : _("after"),
	    uhp == NULL ? 0L : uhp->uh_seq,
	    msgbuf);
}