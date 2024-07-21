static int mailbox_read_header(struct mailbox *mailbox)
{
    int r = 0;
    int flag;
    const char *name, *p, *tab, *eol;
    const char *fname;
    struct stat sbuf;
    const char *base = NULL;
    size_t len = 0;
    unsigned magic_size = sizeof(MAILBOX_HEADER_MAGIC) - 1;

    /* can't be dirty if we're reading it */
    if (mailbox->header_dirty)
        abort();

    xclose(mailbox->header_fd);

    fname = mailbox_meta_fname(mailbox, META_HEADER);
    mailbox->header_fd = open(fname, O_RDONLY, 0);

    if (mailbox->header_fd == -1) {
        r = IMAP_IOERROR;
        goto done;
    }

    if (fstat(mailbox->header_fd, &sbuf) == -1) {
        xclose(mailbox->header_fd);
        r = IMAP_IOERROR;
        goto done;
    }

    map_refresh(mailbox->header_fd, 1, &base, &len,
                sbuf.st_size, "header", mailbox_name(mailbox));
    mailbox->header_file_ino = sbuf.st_ino;
    mailbox->header_file_crc = crc32_map(base, sbuf.st_size);

    /* Check magic number */
    if ((unsigned) sbuf.st_size < magic_size ||
        strncmp(base, MAILBOX_HEADER_MAGIC, magic_size)) {
        r = IMAP_MAILBOX_BADFORMAT;
        goto done;
    }

    /* Read quota data line */
    p = base + sizeof(MAILBOX_HEADER_MAGIC)-1;
    tab = memchr(p, '\t', sbuf.st_size - (p - base));
    eol = memchr(p, '\n', sbuf.st_size - (p - base));
    if (!eol) {
        r = IMAP_MAILBOX_BADFORMAT;
        goto done;
    }

    xzfree(mailbox->h.quotaroot);
    xzfree(mailbox->h.uniqueid);

    /* check for DLIST mboxlist */
    if (*p == '%') {
        r = _parse_header_data(mailbox, p, eol - p);
        goto done;
    }

    /* quotaroot (if present) */
    if (!tab || tab > eol) {
        syslog(LOG_DEBUG, "mailbox '%s' has old cyrus.header",
               mailbox_name(mailbox));
        tab = eol;
    }
    if (p < tab) {
        mailbox->h.quotaroot = xstrndup(p, tab - p);
    }

    /* read uniqueid (should always exist unless old format) */
    if (tab < eol) {
        p = tab + 1;
        if (p == eol) {
            r = IMAP_MAILBOX_BADFORMAT;
            goto done;
        }
        tab = memchr(p, '\t', sbuf.st_size - (p - base));
        if (!tab || tab > eol) tab = eol;
        mailbox->h.uniqueid = xstrndup(p, tab - p);
    }
    else {
        /* ancient cyrus.header file without a uniqueid field! */
        xsyslog(LOG_ERR, "mailbox header has no uniqueid, needs reconstruct",
                         "mboxname=<%s>",
                         mailbox_name(mailbox));
    }

    /* Read names of user flags */
    p = eol + 1;
    eol = memchr(p, '\n', sbuf.st_size - (p - base));
    if (!eol) {
        r = IMAP_MAILBOX_BADFORMAT;
        goto done;
    }
    name = p;
    /* read the names of flags */
    for (flag = 0; name <= eol && flag < MAX_USER_FLAGS; flag++) {
        xzfree(mailbox->h.flagname[flag]);
        p = memchr(name, ' ', eol-name);
        if (!p) p = eol;
        if (name != p)
            mailbox->h.flagname[flag] = xstrndup(name, p-name);
        name = p+1;
    }
    /* zero out the rest */
    for (; flag < MAX_USER_FLAGS; flag++) {
        xzfree(mailbox->h.flagname[flag]);
    }

    /* Read ACL */
    p = eol + 1;
    eol = memchr(p, '\n', sbuf.st_size - (p - base));
    if (!eol) {
        r = IMAP_MAILBOX_BADFORMAT;
        goto done;
    }

    mailbox->h.acl = xstrndup(p, eol-p);

done:
    if (base) map_free(&base, &len);
    return r;
}