static int mailbox_lock_index_internal(struct mailbox *mailbox, int locktype)
{
    struct stat sbuf;
    int r = 0;
    const char *header_fname = mailbox_meta_fname(mailbox, META_HEADER);
    const char *index_fname = mailbox_meta_fname(mailbox, META_INDEX);

    assert(mailbox->index_fd != -1);
    assert(!mailbox->index_locktype);

    char *userid = mboxname_to_userid(mailbox_name(mailbox));
    if (userid) {
        if (!user_isnamespacelocked(userid)) {
            struct mailboxlist *listitem = find_listitem(mailbox_name(mailbox));
            assert(listitem);
            assert(&listitem->m == mailbox);
            r = mailbox_mboxlock_reopen(listitem, LOCK_SHARED, locktype);
            if (locktype == LOCK_SHARED)
                mailbox->is_readonly = 1;
            if (!r) r = mailbox_open_index(mailbox);
        }
        free(userid);
        if (r) return r;
    }

    if (locktype == LOCK_EXCLUSIVE) {
        /* handle read-only case cleanly - we need to re-open read-write first! */
        if (mailbox->is_readonly) {
            mailbox->is_readonly = 0;
            r = mailbox_open_index(mailbox);
        }
        if (!r) r = lock_blocking(mailbox->index_fd, index_fname);
    }
    else if (locktype == LOCK_SHARED) {
        r = lock_shared(mailbox->index_fd, index_fname);
    }
    else {
        /* this function does not support nonblocking locks */
        fatal("invalid locktype for index", EX_SOFTWARE);
    }

    /* double check that the index exists and has at least enough
     * data to check the version number */
    if (!r) {
        if (!mailbox->index_base)
            r = IMAP_MAILBOX_BADFORMAT;
        else if (mailbox->index_size < OFFSET_NUM_RECORDS)
            r = IMAP_MAILBOX_BADFORMAT;
        if (r)
            lock_unlock(mailbox->index_fd, index_fname);
    }

    if (r) {
        xsyslog(LOG_ERR, "IOERROR: lock index failed",
                         "mailbox=<%s> error=<%s>",
                         mailbox_name(mailbox), error_message(r));
        return IMAP_IOERROR;
    }

    mailbox->index_locktype = locktype;
    gettimeofday(&mailbox->starttime, 0);

    r = stat(header_fname, &sbuf);
    if (r == -1) {
        xsyslog(LOG_ERR, "IOERROR: stat header failed",
                         "mailbox=<%s> header=<%s>",
                         mailbox_name(mailbox), header_fname);
        mailbox_unlock_index(mailbox, NULL);
        return IMAP_IOERROR;
    }

    /* has the header file changed? */
    if (sbuf.st_ino != mailbox->header_file_ino) {
        r = mailbox_read_header(mailbox);
        if (r) {
            xsyslog(LOG_ERR, "IOERROR: read header failed",
                             "mailbox=<%s> error=<%s>",
                             mailbox_name(mailbox), error_message(r));
            mailbox_unlock_index(mailbox, NULL);
            return r;
        }
    }

    /* release caches */
    int i;
    for (i = 0; i < mailbox->caches.count; i++) {
        struct mappedfile *cachefile = ptrarray_nth(&mailbox->caches, i);
        mappedfile_close(&cachefile);
    }
    ptrarray_fini(&mailbox->caches);

    /* note: it's guaranteed by our outer cyrus.lock lock that the
     * cyrus.index and cyrus.cache files are never rewritten, so
     * we're safe to just extend the map if needed */
    r = mailbox_read_index_header(mailbox);
    if (r) {
        xsyslog(LOG_ERR, "IOERROR: refreshing index failed",
                         "mailbox=<%s> error=<%s>",
                         mailbox_name(mailbox), error_message(r));
        mailbox_unlock_index(mailbox, NULL);
        return r;
    }

    /* check the CRC */
    if (mailbox->header_file_crc && mailbox->i.header_file_crc &&
        mailbox->header_file_crc != mailbox->i.header_file_crc) {
        syslog(LOG_WARNING, "Header CRC mismatch for mailbox %s: %08X %08X",
               mailbox_name(mailbox), (unsigned int)mailbox->header_file_crc,
               (unsigned int)mailbox->i.header_file_crc);
    }

    return 0;
}