store_word(
    spellinfo_T	*spin,
    char_u	*word,
    int		flags,		// extra flags, WF_BANNED
    int		region,		// supported region(s)
    char_u	*pfxlist,	// list of prefix IDs or NULL
    int		need_affix)	// only store word with affix ID
{
    int		len = (int)STRLEN(word);
    int		ct = captype(word, word + len);
    char_u	foldword[MAXWLEN];
    int		res = OK;
    char_u	*p;

    (void)spell_casefold(curwin, word, len, foldword, MAXWLEN);
    for (p = pfxlist; res == OK; ++p)
    {
	if (!need_affix || (p != NULL && *p != NUL))
	    res = tree_add_word(spin, foldword, spin->si_foldroot, ct | flags,
						  region, p == NULL ? 0 : *p);
	if (p == NULL || *p == NUL)
	    break;
    }
    ++spin->si_foldwcount;

    if (res == OK && (ct == WF_KEEPCAP || (flags & WF_KEEPCAP)))
    {
	for (p = pfxlist; res == OK; ++p)
	{
	    if (!need_affix || (p != NULL && *p != NUL))
		res = tree_add_word(spin, word, spin->si_keeproot, flags,
						  region, p == NULL ? 0 : *p);
	    if (p == NULL || *p == NUL)
		break;
	}
	++spin->si_keepwcount;
    }
    return res;
}