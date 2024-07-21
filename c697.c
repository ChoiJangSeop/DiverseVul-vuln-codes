static void cmdloop(void)
{
    int c, r = 0, mode;
    static struct buf cmd, arg1, arg2, arg3, arg4;
    char *p, *result, buf[1024];
    const char *err;
    uint32_t uid, last;
    struct backend *be;
    char curgroup[MAX_MAILBOX_BUFFER] = "";

    allowanonymous = config_getswitch(IMAPOPT_ALLOWANONYMOUSLOGIN);

    for (;;) {
	/* Flush any buffered output */
	prot_flush(nntp_out);
	if (backend_current) prot_flush(backend_current->out);

	/* Check for shutdown file */
	if (shutdown_file(buf, sizeof(buf)) ||
	    (nntp_userid &&
	     userdeny(nntp_userid, config_ident, buf, sizeof(buf)))) {
	    prot_printf(nntp_out, "400 %s\r\n", buf);
	    shut_down(0);
	}

	signals_poll();

	if (!proxy_check_input(protin, nntp_in, nntp_out,
			       backend_current ? backend_current->in : NULL,
			       NULL, 0)) {
	    /* No input from client */
	    continue;
	}

	if (group_state &&
	    config_getswitch(IMAPOPT_DISCONNECT_ON_VANISHED_MAILBOX)) {
	    if (group_state->mailbox->i.options & OPT_MAILBOX_DELETED) {
		/* Mailbox has been (re)moved */
		syslog(LOG_WARNING,
		       "Newsgroup %s has been (re)moved out from under client",
		       group_state->mailbox->name);
		prot_printf(nntp_out,
			    "400 Newsgroup has been (re)moved\r\n");
		shut_down(0);
	    }
	}

	/* Parse command name */
	c = getword(nntp_in, &cmd);
	if (c == EOF) {
	    if ((err = prot_error(nntp_in)) != NULL
		 && strcmp(err, PROT_EOF_STRING)) {
		syslog(LOG_WARNING, "%s, closing connection", err);
		prot_printf(nntp_out, "400 %s\r\n", err);
	    }
	    return;
	}
	if (!cmd.s[0]) {
	    prot_printf(nntp_out, "501 Empty command\r\n");
	    eatline(nntp_in, c);
	    continue;
	}
	if (Uislower(cmd.s[0])) 
	    cmd.s[0] = toupper((unsigned char) cmd.s[0]);
	for (p = &cmd.s[1]; *p; p++) {
	    if (Uisupper(*p)) *p = tolower((unsigned char) *p);
	}

	/* Ihave/Takethis only allowed for feeders */
	if (!(nntp_capa & MODE_FEED) &&
	    strchr("IT", cmd.s[0])) goto noperm;
    
	/* Body/Date/Group/Newgroups/Newnews/Next/Over/Post/Xhdr/Xover/Xpat
	   only allowed for readers */
	if (!(nntp_capa & MODE_READ) &&
	    strchr("BDGNOPX", cmd.s[0])) goto noperm;
    
	/* Only Authinfo/Capabilities/Check/Head/Help/Ihave/List Active/
	   Mode/Quit/Starttls/Stat/Takethis allowed when not logged in */
	if (!nntp_userid && !allowanonymous &&
	    !strchr("ACHILMQST", cmd.s[0])) goto nologin;

	/* In case a [LIST]GROUP fails or
	   a retrieval by msgid makes us switch groups */
	strcpy(curgroup, group_state ? group_state->mailbox->name : "");

	switch (cmd.s[0]) {
	case 'A':
	    if (!strcmp(cmd.s, "Authinfo")) {
		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg1); /* subcommand */
		if (c == EOF) goto missingargs;

		lcase(arg1.s);

		if (!strcmp(arg1.s, "user") || !strcmp(arg1.s, "pass")) {
		    if (c != ' ') goto missingargs;
		    c = getuserpass(nntp_in, &arg2); /* user/pass */
		    if (c == EOF) goto missingargs;

		    if (c == '\r') c = prot_getc(nntp_in);
		    if (c != '\n') goto extraargs;

		    if (arg1.s[0] == 'u')
			cmd_authinfo_user(arg2.s);
		    else
			cmd_authinfo_pass(arg2.s);
		}
		else if (!strcmp(arg1.s, "sasl") || !strcmp(arg1.s, "generic")) {
		    arg2.len = arg3.len = 0;

		    /* mech name required for SASL but not GENERIC */
		    if ((arg1.s[0] == 's') && (c != ' ')) goto missingargs;

		    if (c == ' ') {
			c = getword(nntp_in, &arg2); /* mech name */
			if (c == EOF) goto missingargs;

			if (c == ' ') {
			    c = getword(nntp_in, &arg3); /* init response */
			    if (c == EOF) goto missingargs;
			}
		    }

		    if (c == '\r') c = prot_getc(nntp_in);
		    if (c != '\n') goto extraargs;

		    cmd_authinfo_sasl(arg1.s, arg2.len ? arg2.s : NULL,
				      arg3.len ? arg3.s : NULL);
		}
		else
		    prot_printf(nntp_out,
				"501 Unrecognized AUTHINFO command\r\n");
	    }
	    else if (!(nntp_capa & MODE_READ)) goto noperm;
	    else if (!nntp_userid && !allowanonymous) goto nologin;
	    else if (!strcmp(cmd.s, "Article")) {
		char *msgid;

		mode = ARTICLE_ALL;

	      article:
		if (arg1.s) *arg1.s = 0;

		if (c == ' ') {
		    c = getword(nntp_in, &arg1); /* number/msgid (optional) */
		    if (c == EOF) goto missingargs;
		}
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		if (parserange(arg1.s, &uid, NULL, &msgid, &be) != -1) {
		    if (be) {
			if (arg1.s && *arg1.s)
			    prot_printf(be->out, "%s %s\r\n", cmd.s, arg1.s);
			else
			    prot_printf(be->out, "%s\r\n", cmd.s);

			if (be != backend_current) {
			    r = read_response(be, 0, &result);
			    if (r) goto noopengroup;

			    prot_printf(nntp_out, "%s", result);
			    if (!strncmp(result, "22", 2) &&
				mode != ARTICLE_STAT) {
				pipe_to_end_of_response(be, 0);
			    }
			}
		    }
		    else
			cmd_article(mode, msgid, uid);
		}

		if (msgid) goto prevgroup;
	    }
	    else goto badcmd;
	    break;

	case 'B':
	    if (!strcmp(cmd.s, "Body")) {
		mode = ARTICLE_BODY;
		goto article;
	    }
	    else goto badcmd;
	    break;

	case 'C':
	    if (!strcmp(cmd.s, "Capabilities")) {
		arg1.len = 0;

		if (c == ' ') {
		    c = getword(nntp_in, &arg1); /* keyword (optional) */
		    if (c == EOF) goto missingargs;
		}
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		cmd_capabilities(arg1.s);
	    }
	    else if (!(nntp_capa & MODE_FEED)) goto noperm;
	    else if (!strcmp(cmd.s, "Check")) {
		mode = POST_CHECK;
		goto ihave;
	    }
	    else goto badcmd;
	    break;

	case 'D':
	    if (!strcmp(cmd.s, "Date")) {
		time_t now = time(NULL);
		struct tm *my_tm = gmtime(&now);
		char buf[15];

		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", my_tm);
		prot_printf(nntp_out, "111 %s\r\n", buf);
	    }
	    else goto badcmd;
	    break;

	case 'G':
	    if (!strcmp(cmd.s, "Group")) {
		arg2.len = 0; /* GROUP command (no range) */

	      group:
#define LISTGROUP (arg2.len)

		if (!LISTGROUP && c != ' ') goto missingargs;
		if (c == ' ') {
		    c = getword(nntp_in, &arg1); /* group */
		    if (c == EOF) goto missingargs;
		}
		if (LISTGROUP && c == ' ') {
		    c = getword(nntp_in, &arg2); /* range (optional) */
		    if (c == EOF) goto missingargs;
		}
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		be = backend_current;
		if (arg1.len &&
		    (!is_newsgroup(arg1.s) ||
		     (r = open_group(arg1.s, 1, &be, NULL)))) goto nogroup;
		else if (be) {
		    prot_printf(be->out, "%s", cmd.s);
		    if (arg1.len) {
			prot_printf(be->out, " %s", arg1.s);
			  if (LISTGROUP) prot_printf(be->out, " %s", arg2.s);
		    }
		    prot_printf(be->out, "\r\n");

		    r = read_response(be, 0, &result);
		    if (r) goto nogroup;

		    prot_printf(nntp_out, "%s", result);

		    if (!strncmp(result, "211", 3)) {
			if (LISTGROUP) pipe_to_end_of_response(be, 0);

			if (backend_current && backend_current != be) {
			    /* remove backend_current from the protgroup */
			    protgroup_delete(protin, backend_current->in);
			}
			backend_current = be;

			/* add backend_current to the protgroup */
			protgroup_insert(protin, backend_current->in);
		    }
		}
		else if (!group_state) goto noopengroup;
		else if (LISTGROUP &&
			 parserange(arg2.s, &uid, &last, NULL, NULL) != 0) {
		    /* parserange() will handle error code -- do nothing */
		}
		else {
		    if (backend_current) {
			/* remove backend_current from the protgroup */
			protgroup_delete(protin, backend_current->in);
		    }
		    backend_current = NULL;

		    nntp_exists = group_state->exists;
		    nntp_current = nntp_exists > 0;

		    prot_printf(nntp_out, "211 %u %lu %lu %s\r\n",
				nntp_exists,
				nntp_exists ? index_getuid(group_state, 1) :
				group_state->last_uid+1,
				nntp_exists ? index_getuid(group_state, nntp_exists) :
				group_state->last_uid,
				group_state->mailbox->name + strlen(newsprefix));

		    if (LISTGROUP) {
			int msgno, last_msgno;

			msgno = index_finduid(group_state, uid);
			if (!msgno || index_getuid(group_state, msgno) != uid) {
			    msgno++;
			}
			last_msgno = index_finduid(group_state, last);

			for (; msgno <= last_msgno; msgno++) {
			    prot_printf(nntp_out, "%u\r\n",
					index_getuid(group_state, msgno));
			}
			prot_printf(nntp_out, ".\r\n");
		    }
		}
#undef LISTGROUP
	    }
	    else goto badcmd;
	    break;

	case 'H':
	    if (!strcmp(cmd.s, "Head")) {
		mode = ARTICLE_HEAD;
		goto article;
	    }
	    else if (!strcmp(cmd.s, "Help")) {
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		cmd_help();
	    }
	    else if (!(nntp_capa & MODE_READ)) goto noperm;
	    else if (!nntp_userid && !allowanonymous) goto nologin;
	    else if (!strcmp(cmd.s, "Hdr")) {
		char *msgid;

	      hdr:
		if (arg2.s) *arg2.s = 0;

		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg1); /* header */
		if (c == EOF) goto missingargs;
		if (c == ' ') {
		    c = getword(nntp_in, &arg2); /* range (optional) */
		    if (c == EOF) goto missingargs;
		}
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		if (parserange(arg2.s, &uid, &last, &msgid, &be) != -1) {
		    if (be) {
			if (arg2.s && *arg2.s)
			    prot_printf(be->out, "%s %s %s\r\n",
					cmd.s, arg1.s, arg2.s);
			else
			    prot_printf(be->out, "%s %s\r\n", cmd.s, arg1.s);

			if (be != backend_current) {
			    r = read_response(be, 0, &result);
			    if (r) goto noopengroup;

			    prot_printf(nntp_out, "%s", result);
			    if (!strncmp(result, "22", 2)) { /* 221 or 225 */
				pipe_to_end_of_response(be, 0);
			    }
			}
		    }
		    else
			cmd_hdr(cmd.s, arg1.s, NULL, msgid, uid, last);
		}

		if (msgid) goto prevgroup;
	    }
	    else goto badcmd;
	    break;

	case 'I':
	    if (!strcmp(cmd.s, "Ihave")) {
		mode = POST_IHAVE;

	      ihave:
		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg1); /* msgid */
		if (c == EOF) goto missingargs;
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		cmd_post(arg1.s, mode);
	    }
	    else goto badcmd;
	    break;

	case 'L':
	    if (!strcmp(cmd.s, "List")) {
		arg1.len = arg2.len = 0;
		if (c == ' ') {
		    c = getword(nntp_in, &arg1); /* subcommand (optional) */
		    if (c == EOF) goto missingargs;
		    if (c == ' ') {
			c = getword(nntp_in, &arg2); /* argument (optional) */
			if (c == EOF) goto missingargs;
		    }
		}
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		cmd_list(arg1.len ? arg1.s : NULL, arg2.len ? arg2.s : NULL);

		goto prevgroup;  /* In case we did LIST [ACTIVE] */
	    }
	    else if (!(nntp_capa & MODE_READ)) goto noperm;
	    else if (!nntp_userid && !allowanonymous) goto nologin;
	    else if (!strcmp(cmd.s, "Last")) {
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		if (backend_current) {
		    prot_printf(backend_current->out, "LAST\r\n");
		}
		else if (!group_state) goto noopengroup;
		else if (!nntp_current) goto nocurrent;
		else if (nntp_current == 1) {
		    prot_printf(nntp_out,
				"422 No previous article in this group\r\n");
		}
		else {
		    char *msgid = index_get_msgid(group_state, --nntp_current);

		    prot_printf(nntp_out, "223 %u %s\r\n",
				index_getuid(group_state, nntp_current),
				msgid ? msgid : "<0>");

		    if (msgid) free(msgid);
		}
	    }
	    else if (!strcmp(cmd.s, "Listgroup")) {
		arg1.len = 0;   	   /* group is optional */
		buf_setcstr(&arg2, "1-");  /* default range is all */
		buf_cstring(&arg2);	   /* appends a '\0' */
		goto group;
	    }
	    else goto badcmd;
	    break;

	case 'M':
	    if (!strcmp(cmd.s, "Mode")) {
		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg1); /* mode */
		if (c == EOF) goto missingargs;
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		cmd_mode(arg1.s);
	    }
	    else goto badcmd;
	    break;

	case 'N':
	    if (!strcmp(cmd.s, "Newgroups")) {
		time_t tstamp;

		arg3.len = 0;
		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg1); /* date */
		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg2); /* time */
		if (c == EOF) goto missingargs;
		if (c == ' ') {
		    c = getword(nntp_in, &arg3); /* "GMT" (optional) */
		    if (c == EOF) goto missingargs;
		}
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		if ((tstamp = parse_datetime(arg1.s, arg2.s,
					     arg3.len ? arg3.s : NULL)) < 0)
		    goto baddatetime;

		cmd_newgroups(tstamp);
	    }
	    else if (!strcmp(cmd.s, "Newnews")) {
		time_t tstamp;

		if (!config_getswitch(IMAPOPT_ALLOWNEWNEWS))
		    goto cmddisabled;

		arg4.len = 0;
		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg1); /* wildmat */
		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg2); /* date */
		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg3); /* time */
		if (c == EOF) goto missingargs;
		if (c == ' ') {
		    c = getword(nntp_in, &arg4); /* "GMT" (optional) */
		    if (c == EOF) goto missingargs;
		}
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		if ((tstamp = parse_datetime(arg2.s, arg3.s,
					     arg4.len ? arg4.s : NULL)) < 0)
		    goto baddatetime;

		cmd_newnews(arg1.s, tstamp);
	    }
	    else if (!strcmp(cmd.s, "Next")) {
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		if (backend_current) {
		    prot_printf(backend_current->out, "NEXT\r\n");
		}
		else if (!group_state) goto noopengroup;
		else if (!nntp_current) goto nocurrent;
		else if (nntp_current == nntp_exists) {
		    prot_printf(nntp_out,
				"421 No next article in this group\r\n");
		}
		else {
		    char *msgid = index_get_msgid(group_state, ++nntp_current);

		    prot_printf(nntp_out, "223 %u %s\r\n",
				index_getuid(group_state, nntp_current),
				msgid ? msgid : "<0>");

		    if (msgid) free(msgid);
		}
	    }
	    else goto badcmd;
	    break;

	case 'O':
	    if (!strcmp(cmd.s, "Over")) {
		char *msgid;

	      over:
		if (arg1.s) *arg1.s = 0;

		if (c == ' ') {
		    c = getword(nntp_in, &arg1); /* range/msgid (optional) */
		    if (c == EOF) goto missingargs;
		}
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		msgid = NULL;
		if (parserange(arg1.s, &uid, &last,
			       /* XOVER doesn't accept message-id */
			       (cmd.s[0] == 'X' ? NULL : &msgid), &be) != -1) {
		    if (be) {
			if (arg1.s && *arg1.s)
			    prot_printf(be->out, "%s %s\r\n", cmd.s, arg1.s);
			else
			    prot_printf(be->out, "%s\r\n", cmd.s);

			if (be != backend_current) {
			    r = read_response(be, 0, &result);
			    if (r) goto noopengroup;

			    prot_printf(nntp_out, "%s", result);
			    if (!strncmp(result, "224", 3)) {
				pipe_to_end_of_response(be, 0);
			    }
			}
		    }
		    else
			cmd_over(msgid, uid, last);
		}

		if (msgid) goto prevgroup;
	    }
	    else goto badcmd;
	    break;

	case 'P':
	    if (!strcmp(cmd.s, "Post")) {
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		cmd_post(NULL, POST_POST);
	    }
	    else goto badcmd;
	    break;

	case 'Q':
	    if (!strcmp(cmd.s, "Quit")) {
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		prot_printf(nntp_out, "205 Connection closing\r\n");
		return;
	    }
	    else goto badcmd;
	    break;

	case 'S':
	    if (!strcmp(cmd.s, "Starttls") && tls_enabled()) {
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		/* XXX  discard any input pipelined after STARTTLS */
		prot_flush(nntp_in);

		cmd_starttls(0);
	    }
	    else if (!strcmp(cmd.s, "Stat")) {
		mode = ARTICLE_STAT;
		goto article;
	    }
	    else if (!nntp_userid && !allowanonymous) goto nologin;
	    else if (!strcmp(cmd.s, "Slave")) {	
		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		prot_printf(nntp_out, "202 Slave status noted\r\n");
	    }
	    else goto badcmd;
	    break;

	case 'T':
	    if (!strcmp(cmd.s, "Takethis")) {
		mode = POST_TAKETHIS;
		goto ihave;
	    }
	    else goto badcmd;
	    break;

	case 'X':
	    if (!strcmp(cmd.s, "Xhdr")) {
		goto hdr;
	    }
	    else if (!strcmp(cmd.s, "Xover")) {
		goto over;
	    }
	    else if (!strcmp(cmd.s, "Xpat")) {
		char *msgid;

		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg1); /* header */
		if (c != ' ') goto missingargs;

		/* gobble extra whitespace (hack for Mozilla) */
		while ((c = prot_getc(nntp_in)) == ' ');
		prot_ungetc(c, nntp_in);

		c = getword(nntp_in, &arg2); /* range */
		if (c != ' ') goto missingargs;
		c = getword(nntp_in, &arg3); /* wildmat */
		if (c == EOF) goto missingargs;

		/* XXX per RFC 2980, we can have multiple patterns */

		if (c == '\r') c = prot_getc(nntp_in);
		if (c != '\n') goto extraargs;

		if (parserange(arg2.s, &uid, &last, &msgid, &be) != -1) {
		    if (be) {
			prot_printf(be->out, "%s %s %s %s\r\n",
				    cmd.s, arg1.s, arg2.s, arg3.s);

			if (be != backend_current) {
			    r = read_response(be, 0, &result);
			    if (r) goto noopengroup;

			    prot_printf(nntp_out, "%s", result);
			    if (!strncmp(result, "221", 3)) {
				pipe_to_end_of_response(be, 0);
			    }
			}
		    }
		    else
			cmd_hdr(cmd.s, arg1.s, arg3.s, msgid, uid, last);
		}

		if (msgid) goto prevgroup;
	    }
	    else goto badcmd;
	    break;

	default:
	  badcmd:
	    prot_printf(nntp_out, "500 Unrecognized command\r\n");
	    eatline(nntp_in, c);
	}

	continue;

      noperm:
	prot_printf(nntp_out, "502 Permission denied\r\n");
	eatline(nntp_in, c);
	continue;

      nologin:
	prot_printf(nntp_out, "480 Authentication required\r\n");
	eatline(nntp_in, c);
	continue;

      cmddisabled:
	prot_printf(nntp_out, "503 \"%s\" disabled\r\n", cmd.s);
	eatline(nntp_in, c);
	continue;

      extraargs:
	prot_printf(nntp_out, "501 Unexpected extra argument\r\n");
	eatline(nntp_in, c);
	continue;

      missingargs:
	prot_printf(nntp_out, "501 Missing argument\r\n");
	eatline(nntp_in, c);
	continue;

      baddatetime:
	prot_printf(nntp_out, "501 Bad date/time\r\n");
	continue;

      nogroup:
	prot_printf(nntp_out, "411 No such newsgroup (%s)\r\n",
		    error_message(r));

      prevgroup:
	/* Return to previously selected group */
	if (*curgroup &&
	    (!group_state || strcmp(curgroup, group_state->mailbox->name))) {
	    open_group(curgroup, 1, NULL, NULL);
	}

	continue;

      noopengroup:
	prot_printf(nntp_out, "412 No newsgroup selected\r\n");
	continue;

      nocurrent:
	prot_printf(nntp_out, "420 Current article number is invalid\r\n");
	continue;
    }
}