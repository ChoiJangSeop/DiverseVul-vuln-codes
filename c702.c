static int deliver_remote(message_data_t *msg, struct dest *dlist)
{
    struct dest *d;

    /* run the txns */
    for (d = dlist; d; d = d->next) {
	struct backend *be;
	char buf[4096];

	be = proxy_findserver(d->server, &nntp_protocol,
			      nntp_userid ? nntp_userid : "anonymous",
			      &backend_cached, &backend_current,
			      NULL, nntp_in);
	if (!be) return IMAP_SERVER_UNAVAILABLE;

	/* tell the backend about our new article */
	prot_printf(be->out, "IHAVE %s\r\n", msg->id);
	prot_flush(be->out);

	if (!prot_fgets(buf, sizeof(buf), be->in) ||
	    strncmp("335", buf, 3)) {
	    syslog(LOG_NOTICE, "backend doesn't want article %s", msg->id);
	    continue;
	}

	/* send the article */
	rewind(msg->f);
	while (fgets(buf, sizeof(buf), msg->f)) {
	    if (buf[0] == '.') prot_putc('.', be->out);
	    do {
		prot_printf(be->out, "%s", buf);
	    } while (buf[strlen(buf)-1] != '\n' &&
		     fgets(buf, sizeof(buf), msg->f));
	}

	/* Protect against messages not ending in CRLF */
	if (buf[strlen(buf)-1] != '\n') prot_printf(be->out, "\r\n");

	prot_printf(be->out, ".\r\n");

	if (!prot_fgets(buf, sizeof(buf), be->in) ||
	    strncmp("235", buf, 3)) {
	    syslog(LOG_WARNING, "article %s transfer to backend failed",
		   msg->id);
	    return NNTP_FAIL_TRANSFER;
	}
    }

    return 0;
}