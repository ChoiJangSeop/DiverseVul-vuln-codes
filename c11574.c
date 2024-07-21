diff_buf_delete(buf_T *buf)
{
    int		i;
    tabpage_T	*tp;

    FOR_ALL_TABPAGES(tp)
    {
	i = diff_buf_idx_tp(buf, tp);
	if (i != DB_COUNT)
	{
	    tp->tp_diffbuf[i] = NULL;
	    tp->tp_diff_invalid = TRUE;
	    if (tp == curtab)
		diff_redraw(TRUE);
	}
    }
}