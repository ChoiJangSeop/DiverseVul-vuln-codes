EXPORTED void mailbox_make_uniqueid(struct mailbox *mailbox)
{
    free(mailbox->h.uniqueid);
    mailbox->h.uniqueid = xstrdup(makeuuid());
    mailbox->header_dirty = 1;
}