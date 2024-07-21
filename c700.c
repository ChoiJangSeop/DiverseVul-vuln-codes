static void cmd_help(void)
{
    prot_printf(nntp_out, "100 Supported commands:\r\n");

    if ((nntp_capa & MODE_READ) && (nntp_userid || allowanonymous)) {
	prot_printf(nntp_out, "\tARTICLE [ message-id | number ]\r\n"
		    "\t\tRetrieve entirety of the specified article.\r\n");
    }
    if (!nntp_authstate) {
	if (!nntp_userid) {
	    prot_printf(nntp_out, "\tAUTHINFO SASL mechanism [initial-response]\r\n"
			"\t\tPerform an authentication exchange using the specified\r\n"
			"\t\tSASL mechanism.\r\n");
	    prot_printf(nntp_out, "\tAUTHINFO USER username\r\n"
			"\t\tPresent username for authentication.\r\n");
	}
	prot_printf(nntp_out, "\tAUTHINFO PASS password\r\n"
		    "\t\tPresent clear-text password for authentication.\r\n");
    }
    if ((nntp_capa & MODE_READ) && (nntp_userid || allowanonymous)) {
	prot_printf(nntp_out, "\tBODY [ message-id | number ]\r\n"
		    "\t\tRetrieve body of the specified article.\r\n");
    }
    prot_printf(nntp_out, "\tCAPABILITIES\r\n"
		"\t\tList the current server capabilities.\r\n");
    if (nntp_capa & MODE_FEED) {
	prot_printf(nntp_out, "\tCHECK message-id\r\n"
		    "\t\tCheck if the server wants the specified article.\r\n");
    }
    if ((nntp_capa & MODE_READ) && (nntp_userid || allowanonymous)) {
	prot_printf(nntp_out, "\tDATE\r\n"
		    "\t\tRequest the current server UTC date and time.\r\n");
	prot_printf(nntp_out, "\tGROUP group\r\n"
		    "\t\tSelect a newsgroup for article retrieval.\r\n");
	prot_printf(nntp_out, "\tHDR header [ message-id | range ]\r\n"
		    "\t\tRetrieve the specified header/metadata from the\r\n"
		    "\t\tspecified article(s).\r\n");
    }
    prot_printf(nntp_out, "\tHEAD [ message-id | number ]\r\n"
		"\t\tRetrieve the headers of the specified article.\r\n");
    prot_printf(nntp_out, "\tHELP\r\n"
		"\t\tRequest command summary (this text).\r\n");
    if (nntp_capa & MODE_FEED) {
	prot_printf(nntp_out, "\tIHAVE message-id\r\n"
		    "\t\tPresent/transfer the specified article to the server.\r\n");
    }
    if ((nntp_capa & MODE_READ) && (nntp_userid || allowanonymous)) {
	prot_printf(nntp_out, "\tLAST\r\n"
		    "\t\tSelect the previous article.\r\n");
    }
    prot_printf(nntp_out, "\tLIST [ ACTIVE wildmat ]\r\n"
		"\t\tList the (subset of) valid newsgroups.\r\n");
    if ((nntp_capa & MODE_READ) && (nntp_userid || allowanonymous)) {
	prot_printf(nntp_out, "\tLIST HEADERS [ MSGID | RANGE ]\r\n"
		    "\t\tList the headers and metadata items available via HDR.\r\n");
	prot_printf(nntp_out, "\tLIST NEWSGROUPS [wildmat]\r\n"
		    "\t\tList the descriptions of the specified newsgroups.\r\n");
	prot_printf(nntp_out, "\tLIST OVERVIEW.FMT\r\n"
		    "\t\tList the headers and metadata items available via OVER.\r\n");
	prot_printf(nntp_out, "\tLISTGROUP [group [range]]\r\n"
		    "\t\tList the article numbers in the specified newsgroup.\r\n");
	if (config_getswitch(IMAPOPT_ALLOWNEWNEWS))
	    prot_printf(nntp_out, "\tNEWNEWS wildmat date time [GMT]\r\n"
			"\t\tList the newly arrived articles in the specified newsgroup(s)\r\n"
			"\t\tsince the specified date and time.\r\n");
	prot_printf(nntp_out, "\tNEXT\r\n"
		    "\t\tSelect the next article.\r\n");
	prot_printf(nntp_out, "\tOVER [ message-id | range ]\r\n"
		    "\t\tRetrieve the overview information for the specified article(s).\r\n");
	prot_printf(nntp_out, "\tPOST\r\n"
		    "\t\tPost an article to the server.\r\n");
    }

    prot_printf(nntp_out, "\tQUIT\r\n"
		"\t\tTerminate the session.\r\n");
    if (tls_enabled() && !nntp_starttls_done && !nntp_authstate) {
	prot_printf(nntp_out, "\tSTARTTLS\r\n"
		    "\t\tStart a TLS negotiation.\r\n");
    }
    prot_printf(nntp_out, "\tSTAT [ message-id | number ]\r\n"
		"\t\tCheck if the specified article exists.\r\n");
    if (nntp_capa & MODE_FEED) {
	prot_printf(nntp_out, "\tTAKETHIS message-id\r\n"
		    "\t\tTransfer the specified article to the server.\r\n");
    }
    if ((nntp_capa & MODE_READ) && (nntp_userid || allowanonymous)) {
	prot_printf(nntp_out, "\tXPAT header message-id|range wildmat\r\n"
		    "\t\tList the specified article(s) in which the contents\r\n"
		    "\t\tof the specified header/metadata matches the wildmat.\r\n");
    }
    prot_printf(nntp_out, ".\r\n");
}