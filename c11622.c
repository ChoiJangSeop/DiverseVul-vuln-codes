ins_compl_infercase_gettext(
	char_u	*str,
	int	actual_len,
	int	actual_compl_length,
	int	min_len)
{
    int		*wca;			// Wide character array.
    char_u	*p;
    int		i, c;
    int		has_lower = FALSE;
    int		was_letter = FALSE;

    IObuff[0] = NUL;

    // Allocate wide character array for the completion and fill it.
    wca = ALLOC_MULT(int, actual_len);
    if (wca == NULL)
	return IObuff;

    p = str;
    for (i = 0; i < actual_len; ++i)
	if (has_mbyte)
	    wca[i] = mb_ptr2char_adv(&p);
	else
	    wca[i] = *(p++);

    // Rule 1: Were any chars converted to lower?
    p = compl_orig_text;
    for (i = 0; i < min_len; ++i)
    {
	if (has_mbyte)
	    c = mb_ptr2char_adv(&p);
	else
	    c = *(p++);
	if (MB_ISLOWER(c))
	{
	    has_lower = TRUE;
	    if (MB_ISUPPER(wca[i]))
	    {
		// Rule 1 is satisfied.
		for (i = actual_compl_length; i < actual_len; ++i)
		    wca[i] = MB_TOLOWER(wca[i]);
		break;
	    }
	}
    }

    // Rule 2: No lower case, 2nd consecutive letter converted to
    // upper case.
    if (!has_lower)
    {
	p = compl_orig_text;
	for (i = 0; i < min_len; ++i)
	{
	    if (has_mbyte)
		c = mb_ptr2char_adv(&p);
	    else
		c = *(p++);
	    if (was_letter && MB_ISUPPER(c) && MB_ISLOWER(wca[i]))
	    {
		// Rule 2 is satisfied.
		for (i = actual_compl_length; i < actual_len; ++i)
		    wca[i] = MB_TOUPPER(wca[i]);
		break;
	    }
	    was_letter = MB_ISLOWER(c) || MB_ISUPPER(c);
	}
    }

    // Copy the original case of the part we typed.
    p = compl_orig_text;
    for (i = 0; i < min_len; ++i)
    {
	if (has_mbyte)
	    c = mb_ptr2char_adv(&p);
	else
	    c = *(p++);
	if (MB_ISLOWER(c))
	    wca[i] = MB_TOLOWER(wca[i]);
	else if (MB_ISUPPER(c))
	    wca[i] = MB_TOUPPER(wca[i]);
    }

    // Generate encoding specific output from wide character array.
    // Multi-byte characters can occupy up to five bytes more than
    // ASCII characters, and we also need one byte for NUL, so stay
    // six bytes away from the edge of IObuff.
    p = IObuff;
    i = 0;
    while (i < actual_len && (p - IObuff + 6) < IOSIZE)
	if (has_mbyte)
	    p += (*mb_char2bytes)(wca[i++], p);
	else
	    *(p++) = wca[i++];
    *p = NUL;

    vim_free(wca);

    return IObuff;
}