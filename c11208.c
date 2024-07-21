get_user_command_name(int idx, int cmdidx)
{
    if (cmdidx == CMD_USER && idx < ucmds.ga_len)
	return USER_CMD(idx)->uc_name;
    if (cmdidx == CMD_USER_BUF)
    {
	// In cmdwin, the alternative buffer should be used.
	buf_T *buf =
#ifdef FEAT_CMDWIN
		    is_in_cmdwin() ? prevwin->w_buffer :
#endif
		    curbuf;

	if (idx < buf->b_ucmds.ga_len)
	    return USER_CMD_GA(&buf->b_ucmds, idx)->uc_name;
    }
    return NULL;
}