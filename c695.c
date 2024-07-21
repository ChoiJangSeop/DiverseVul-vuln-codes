static void cmd_list(char *arg1, char *arg2)
{
    if (!arg1)
	arg1 = "active";
    else
	lcase(arg1);

    if (!strcmp(arg1, "active")) {
	char pattern[MAX_MAILBOX_BUFFER];
	struct list_rock lrock;
	struct enum_rock erock;

	if (!arg2) arg2 = "*";

	erock.cmd = "ACTIVE";
	erock.wild = xstrdup(arg2); /* make a copy before we munge it */

	lrock.proc = do_active;
	lrock.wild = split_wildmats(arg2); /* split the list of wildmats */

	/* xxx better way to determine a size for this table? */
	construct_hash_table(&lrock.server_table, 10, 1);

	prot_printf(nntp_out, "215 List of newsgroups follows:\r\n");

	strcpy(pattern, newsprefix);
	strcat(pattern, "*");
	list_cb(NULL, 0, 0, NULL);
	mboxlist_findall(NULL, pattern, 0, nntp_userid, nntp_authstate,
			 list_cb, &lrock);

	/* proxy to the backends */
	hash_enumerate(&lrock.server_table, list_proxy, &erock);

	prot_printf(nntp_out, ".\r\n");

	/* free the hash table */
	free_hash_table(&lrock.server_table, NULL);

	/* free the wildmats */
	free_wildmats(lrock.wild);
	free(erock.wild);

	if (group_state)
	    index_close(&group_state);
    }
    else if (!(nntp_capa & MODE_READ)) {
	prot_printf(nntp_out, "502 Permission denied\r\n");
	return;
    }
    else if (!nntp_userid && !allowanonymous) {
	prot_printf(nntp_out, "480 Authentication required\r\n");
	return;
    }
    else if (!strcmp(arg1, "headers")) {
	if (arg2 && strcmp(arg2, "msgid") && strcmp(arg2, "range")) {
	    prot_printf(nntp_out, "501 Unexpected extra argument\r\n");
	    return;
	}

	prot_printf(nntp_out, "215 Header and metadata list follows:\r\n");
	prot_printf(nntp_out, ":\r\n"); /* all headers */
	prot_printf(nntp_out, ":bytes\r\n");
	prot_printf(nntp_out, ":lines\r\n");
	prot_printf(nntp_out, ".\r\n");
    }
    else if (!strcmp(arg1, "newsgroups")) {
	char pattern[MAX_MAILBOX_BUFFER];
	struct list_rock lrock;
	struct enum_rock erock;

	if (!arg2) arg2 = "*";

	erock.cmd = "NEWSGROUPS";
	erock.wild = xstrdup(arg2); /* make a copy before we munge it */

	lrock.proc = do_newsgroups;
	lrock.wild = split_wildmats(arg2); /* split the list of wildmats */

	/* xxx better way to determine a size for this table? */
	construct_hash_table(&lrock.server_table, 10, 1);

	prot_printf(nntp_out, "215 List of newsgroups follows:\r\n");

	strcpy(pattern, newsprefix);
	strcat(pattern, "*");
	list_cb(NULL, 0, 0, NULL);
	mboxlist_findall(NULL, pattern, 0, nntp_userid, nntp_authstate,
			 list_cb, &lrock);

	/* proxy to the backends */
	hash_enumerate(&lrock.server_table, list_proxy, &erock);

	strcpy(pattern, newsprefix);
	strcat(pattern, "*");
	annotatemore_findall(pattern, "/comment",
			     newsgroups_cb, lrock.wild, NULL);

	prot_printf(nntp_out, ".\r\n");

	/* free the hash table */
	free_hash_table(&lrock.server_table, NULL);

	/* free the wildmats */
	free_wildmats(lrock.wild);
	free(erock.wild);
    }
    else if (!strcmp(arg1, "overview.fmt")) {
	if (arg2) {
	    prot_printf(nntp_out, "501 Unexpected extra argument\r\n");
	    return;
	}

	prot_printf(nntp_out, "215 Order of overview fields follows:\r\n");
	prot_printf(nntp_out, "Subject:\r\n");
	prot_printf(nntp_out, "From:\r\n");
	prot_printf(nntp_out, "Date:\r\n");
	prot_printf(nntp_out, "Message-ID:\r\n");
	prot_printf(nntp_out, "References:\r\n");
	if (did_capabilities) {
	    /* new OVER format */
	    prot_printf(nntp_out, ":bytes\r\n");
	    prot_printf(nntp_out, ":lines\r\n");
	} else {
	    /* old XOVER format */
	    prot_printf(nntp_out, "Bytes:\r\n");
	    prot_printf(nntp_out, "Lines:\r\n");
	}
	prot_printf(nntp_out, "Xref:full\r\n");
	prot_printf(nntp_out, ".\r\n");
    }
    else if (!strcmp(arg1, "active.times") || !strcmp(arg1, "distributions") ||
	     !strcmp(arg1, "distrib.pats")) {
	prot_printf(nntp_out, "503 Unsupported LIST command\r\n");
    }
    else {
	prot_printf(nntp_out, "501 Unrecognized LIST command\r\n");
    }
    prot_flush(nntp_out);
}