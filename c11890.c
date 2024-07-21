validate_cursor(void)
{
    check_cursor_moved(curwin);
    if ((curwin->w_valid & (VALID_WCOL|VALID_WROW)) != (VALID_WCOL|VALID_WROW))
	curs_columns(TRUE);
}