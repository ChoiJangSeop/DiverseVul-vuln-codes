win_alloc_lines(win_T *wp)
{
    wp->w_lines_valid = 0;
    wp->w_lines = ALLOC_CLEAR_MULT(wline_T, Rows );
    if (wp->w_lines == NULL)
	return FAIL;
    return OK;
}