init_ccline(int firstc, int indent)
{
    ccline.overstrike = FALSE;		    // always start in insert mode

    /*
     * set some variables for redrawcmd()
     */
    ccline.cmdfirstc = (firstc == '@' ? 0 : firstc);
    ccline.cmdindent = (firstc > 0 ? indent : 0);

    // alloc initial ccline.cmdbuff
    alloc_cmdbuff(exmode_active ? 250 : indent + 1);
    if (ccline.cmdbuff == NULL)
	return FAIL;
    ccline.cmdlen = ccline.cmdpos = 0;
    ccline.cmdbuff[0] = NUL;
    sb_text_start_cmdline();

    // autoindent for :insert and :append
    if (firstc <= 0)
    {
	vim_memset(ccline.cmdbuff, ' ', indent);
	ccline.cmdbuff[indent] = NUL;
	ccline.cmdpos = indent;
	ccline.cmdspos = indent;
	ccline.cmdlen = indent;
    }

    return OK;
}