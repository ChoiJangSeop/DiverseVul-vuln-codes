buflist_match(
    regmatch_T	*rmp,
    buf_T	*buf,
    int		ignore_case)  // when TRUE ignore case, when FALSE use 'fic'
{
    char_u	*match;

    // First try the short file name, then the long file name.
    match = fname_match(rmp, buf->b_sfname, ignore_case);
    if (match == NULL)
	match = fname_match(rmp, buf->b_ffname, ignore_case);

    return match;
}