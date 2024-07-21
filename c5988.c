static void cmd_oper_got_pass(const char *password, OPER_PASS_REC *rec)
{
        if (*password != '\0')
		irc_send_cmdv(rec->server, "OPER %s %s", rec->nick, password);
	g_free(rec->nick);
        g_free(rec);
}