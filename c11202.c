goto_tabpage(int n)
{
    tabpage_T	*tp = NULL;  // shut up compiler
    tabpage_T	*ttp;
    int		i;

    if (text_locked())
    {
	// Not allowed when editing the command line.
	text_locked_msg();
	return;
    }

    // If there is only one it can't work.
    if (first_tabpage->tp_next == NULL)
    {
	if (n > 1)
	    beep_flush();
	return;
    }

    if (n == 0)
    {
	// No count, go to next tab page, wrap around end.
	if (curtab->tp_next == NULL)
	    tp = first_tabpage;
	else
	    tp = curtab->tp_next;
    }
    else if (n < 0)
    {
	// "gT": go to previous tab page, wrap around end.  "N gT" repeats
	// this N times.
	ttp = curtab;
	for (i = n; i < 0; ++i)
	{
	    for (tp = first_tabpage; tp->tp_next != ttp && tp->tp_next != NULL;
		    tp = tp->tp_next)
		;
	    ttp = tp;
	}
    }
    else if (n == 9999)
    {
	// Go to last tab page.
	for (tp = first_tabpage; tp->tp_next != NULL; tp = tp->tp_next)
	    ;
    }
    else
    {
	// Go to tab page "n".
	tp = find_tabpage(n);
	if (tp == NULL)
	{
	    beep_flush();
	    return;
	}
    }

    goto_tabpage_tp(tp, TRUE, TRUE);

#ifdef FEAT_GUI_TABLINE
    if (gui_use_tabline())
	gui_mch_set_curtab(tabpage_index(curtab));
#endif
}