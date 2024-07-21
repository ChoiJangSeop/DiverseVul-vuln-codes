static void cmd_capabilities(char *keyword __attribute__((unused)))
{
    const char *mechlist;
    int mechcount = 0;

    prot_printf(nntp_out, "101 Capability list follows:\r\n");
    prot_printf(nntp_out, "VERSION 2\r\n");
    if (nntp_authstate || (config_serverinfo == IMAP_ENUM_SERVERINFO_ON)) {
	prot_printf(nntp_out,
		    "IMPLEMENTATION Cyrus NNTP%s %s\r\n",
		    config_mupdate_server ? " Murder" : "", cyrus_version());
    }

    /* add STARTTLS */
    if (tls_enabled() && !nntp_starttls_done && !nntp_authstate)
	prot_printf(nntp_out, "STARTTLS\r\n");

    /* check for SASL mechs */
    sasl_listmech(nntp_saslconn, NULL, "SASL ", " ", "\r\n",
		  &mechlist, NULL, &mechcount);

    /* add the AUTHINFO variants */
    if (!nntp_authstate) {
	prot_printf(nntp_out, "AUTHINFO%s%s\r\n",
		    (nntp_starttls_done || (extprops_ssf > 1) ||
		     config_getswitch(IMAPOPT_ALLOWPLAINTEXT)) ?
		    " USER" : "", mechcount ? " SASL" : "");
    }

    /* add the SASL mechs */
    if (mechcount) prot_printf(nntp_out, "%s", mechlist);

    /* add the reader capabilities/extensions */
    if ((nntp_capa & MODE_READ) && (nntp_userid || allowanonymous)) {
	prot_printf(nntp_out, "READER\r\n");
	prot_printf(nntp_out, "POST\r\n");
	if (config_getswitch(IMAPOPT_ALLOWNEWNEWS))
	    prot_printf(nntp_out, "NEWNEWS\r\n");
	prot_printf(nntp_out, "HDR\r\n");
	prot_printf(nntp_out, "OVER\r\n");
	prot_printf(nntp_out, "XPAT\r\n");
    }

    /* add the feeder capabilities/extensions */
    if (nntp_capa & MODE_FEED) {
	prot_printf(nntp_out, "IHAVE\r\n");
	prot_printf(nntp_out, "STREAMING\r\n");
    }

    /* add the LIST variants */
    prot_printf(nntp_out, "LIST ACTIVE%s\r\n",
		((nntp_capa & MODE_READ) && (nntp_userid || allowanonymous)) ?
		" HEADERS NEWSGROUPS OVERVIEW.FMT" : "");

    prot_printf(nntp_out, ".\r\n");

    did_capabilities = 1;
}