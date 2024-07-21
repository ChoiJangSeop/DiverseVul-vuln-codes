find_next_quote(
    char_u	*line,
    int		col,
    int		quotechar,
    char_u	*escape)	// escape characters, can be NULL
{
    int		c;

    for (;;)
    {
	c = line[col];
	if (c == NUL)
	    return -1;
	else if (escape != NULL && vim_strchr(escape, c))
	    ++col;
	else if (c == quotechar)
	    break;
	if (has_mbyte)
	    col += (*mb_ptr2len)(line + col);
	else
	    ++col;
    }
    return col;
}