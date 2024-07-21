check_lnums(int do_curwin)
{
    win_T	*wp;
    tabpage_T	*tp;

    FOR_ALL_TAB_WINDOWS(tp, wp)
	if ((do_curwin || wp != curwin) && wp->w_buffer == curbuf)
	{
	    // save the original cursor position and topline
	    wp->w_save_cursor.w_cursor_save = wp->w_cursor;
	    wp->w_save_cursor.w_topline_save = wp->w_topline;

	    if (wp->w_cursor.lnum > curbuf->b_ml.ml_line_count)
		wp->w_cursor.lnum = curbuf->b_ml.ml_line_count;
	    if (wp->w_topline > curbuf->b_ml.ml_line_count)
		wp->w_topline = curbuf->b_ml.ml_line_count;

	    // save the corrected cursor position and topline
	    wp->w_save_cursor.w_cursor_corr = wp->w_cursor;
	    wp->w_save_cursor.w_topline_corr = wp->w_topline;
	}
}