ga_add_string(garray_T *gap, char_u *p)
{
    char_u *cp = vim_strsave(p);

    if (cp == NULL)
	return FAIL;

    if (ga_grow(gap, 1) == FAIL)
    {
	vim_free(cp);
	return FAIL;
    }
    ((char_u **)(gap->ga_data))[gap->ga_len++] = cp;
    return OK;
}