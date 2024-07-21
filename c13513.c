static int mailbox_reconstruct_create(const char *name, struct mailbox **mbptr)
{
    struct mailbox *mailbox = NULL;
    int options = config_getint(IMAPOPT_MAILBOX_DEFAULT_OPTIONS)
                | OPT_POP3_NEW_UIDL;
    mbentry_t *mbentry = NULL;
    struct mailboxlist *listitem;
    int r;

    /* make sure it's not already open.  Very odd, since we already
     * discovered it's not openable! */
    listitem = find_listitem(name);
    if (listitem) return IMAP_MAILBOX_LOCKED;

    listitem = create_listitem(name);
    mailbox = &listitem->m;

    // lock the user namespace FIRST before the mailbox namespace
    char *userid = mboxname_to_userid(name);
    if (userid) {
        int haslock = user_isnamespacelocked(userid);
        if (haslock) {
            assert(haslock != LOCK_SHARED);
        }
        else {
            int locktype = LOCK_EXCLUSIVE;
            mailbox->local_namespacelock = user_namespacelock_full(userid, locktype);
        }
        free(userid);
    }

    /* Start by looking up current data in mailbox list */
    /* XXX - no mboxlist entry?  Can we recover? */
    r = mboxlist_lookup(name, &mbentry, NULL);
    if (r) goto done;

    /* if we can't get an exclusive lock first try, there's something
     * racy going on! */
    uint32_t legacy_dirs = (mbentry->mbtype & MBTYPE_LEGACY_DIRS);
    r = mboxname_lock(legacy_dirs ? name : mbentry->uniqueid, &listitem->l, LOCK_EXCLUSIVE);
    if (r) goto done;

    mailbox->mbentry = mboxlist_entry_copy(mbentry);

    syslog(LOG_NOTICE, "create new mailbox %s", name);

    /* Attempt to open index */
    r = mailbox_open_index(mailbox);
    if (!r) r = mailbox_read_index_header(mailbox);
    if (r) {
        printf("%s: failed to read index header\n", mailbox_name(mailbox));
        syslog(LOG_ERR, "failed to read index header for %s", mailbox_name(mailbox));
        /* no cyrus.index file at all - well, we're in a pickle!
         * no point trying to rescue anything else... */
        mailbox_close(&mailbox);
        r = mailbox_create(name, mbentry->mbtype, mbentry->partition, mbentry->acl,
                           mbentry->uniqueid, options, 0, 0, 0, mbptr);
        mboxlist_entry_free(&mbentry);
        return r;
    }

    mboxlist_entry_free(&mbentry);

    /* read header, if it is not there, we need to create it */
    r = mailbox_read_header(mailbox);
    if (r) {
        /* Header failed to read - recreate it */
        printf("%s: failed to read header file\n", mailbox_name(mailbox));
        syslog(LOG_ERR, "failed to read header file for %s", mailbox_name(mailbox));

        mailbox_make_uniqueid(mailbox);
        r = mailbox_commit(mailbox);
        if (r) goto done;
    }

    if (mailbox->header_file_crc != mailbox->i.header_file_crc) {
        mailbox->i.header_file_crc = mailbox->header_file_crc;
        printf("%s: header file CRC mismatch, correcting\n", mailbox_name(mailbox));
        syslog(LOG_ERR, "%s: header file CRC mismatch, correcting", mailbox_name(mailbox));
        mailbox_index_dirty(mailbox);
        r = mailbox_commit(mailbox);
        if (r) goto done;
    }

done:
    if (r) mailbox_close(&mailbox);
    else *mbptr = mailbox;

    return r;
}