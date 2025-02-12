get_user_commands(expand_T *xp UNUSED, int idx)
{
    // In cmdwin, the alternative buffer should be used.
    buf_T *buf =
#ifdef FEAT_CMDWIN
	is_in_cmdwin() ? prevwin->w_buffer :
#endif
	curbuf;

    if (idx < buf->b_ucmds.ga_len)
	return USER_CMD_GA(&buf->b_ucmds, idx)->uc_name;
    idx -= buf->b_ucmds.ga_len;
    if (idx < ucmds.ga_len)
	return USER_CMD(idx)->uc_name;
    return NULL;
}