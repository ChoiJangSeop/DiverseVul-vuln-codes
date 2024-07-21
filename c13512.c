EXPORTED int mailbox_abort(struct mailbox *mailbox)
{
    int r;

#ifdef WITH_DAV
    r = mailbox_abort_dav(mailbox);
    if (r) return r;
#endif

#ifdef USE_SIEVE
    r = mailbox_abort_sieve(mailbox);
    if (r) return r;
#endif

    /* try to commit sub parts first */
    r = mailbox_abort_cache(mailbox);
    if (r) return r;

    annotate_state_abort(&mailbox->annot_state);

    if (mailbox->local_cstate)
        conversations_abort(&mailbox->local_cstate);

    if (!mailbox->i.dirty)
        return 0;

    assert(mailbox_index_islocked(mailbox, 1));

    /* remove all dirty flags! */
    mailbox->i.dirty = 0;
    mailbox->modseq_dirty = 0;
    mailbox->header_dirty = 0;

    /* removed cached changes */
    _cleanup_changes(mailbox);

    /* we re-read the header and index header to wipe
     * away all the changed values */
    r = mailbox_read_header(mailbox);
    if (r) return r;

    r = mailbox_read_index_header(mailbox);
    if (r) return r;

    return 0;
}