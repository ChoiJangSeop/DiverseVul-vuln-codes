check_restricted(void)
{
    if (restricted)
    {
	emsg(_("E145: Shell commands not allowed in rvim"));
	return TRUE;
    }
    return FALSE;
}