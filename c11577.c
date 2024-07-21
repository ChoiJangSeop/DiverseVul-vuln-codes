check_shellsize(void)
{
    if (Rows < min_rows())	// need room for one window and command line
	Rows = min_rows();
    limit_screen_size();
}