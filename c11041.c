get_function_line(
	exarg_T		*eap,
	char_u		**line_to_free,
	int		indent,
	getline_opt_T	getline_options)
{
    char_u *theline;

    if (eap->getline == NULL)
	theline = getcmdline(':', 0L, indent, 0);
    else
	theline = eap->getline(':', eap->cookie, indent, getline_options);
    if (theline != NULL)
    {
	if (*eap->cmdlinep == *line_to_free)
	    *eap->cmdlinep = theline;
	vim_free(*line_to_free);
	*line_to_free = theline;
    }

    return theline;
}