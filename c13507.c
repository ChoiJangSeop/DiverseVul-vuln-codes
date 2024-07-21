static int mailbox_commit_header(struct mailbox *mailbox)
{
    int fd;
    int r = 0;
    const char *newfname;
    struct iovec iov[10];
    int niov;

    if (!mailbox->header_dirty)
        return 0; /* nothing to write! */

    /* we actually do all header actions under an INDEX lock, because
     * we need to write the crc32 to be consistent! */
    assert(mailbox_index_islocked(mailbox, 1));

    newfname = mailbox_meta_newfname(mailbox, META_HEADER);

    fd = open(newfname, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd == -1) {
        xsyslog(LOG_ERR, "IOERROR: open failed",
                         "newfname=<%s>",
                         newfname);
        return IMAP_IOERROR;
    }

    /* Write magic header, do NOT write the trailing NUL */
    r = write(fd, MAILBOX_HEADER_MAGIC,
              sizeof(MAILBOX_HEADER_MAGIC) - 1);

    if (r != -1) {
        char *data = mailbox_header_data_cstring(mailbox);
        niov = 0;
        WRITEV_ADDSTR_TO_IOVEC(iov, niov, data);
        WRITEV_ADD_TO_IOVEC(iov, niov, "\n", 1);
        r = retry_writev(fd, iov, niov);
        free(data);
    }

    if (r == -1 || fsync(fd)) {
        xsyslog(LOG_ERR, "IOERROR: write failed",
                         "newfname=<%s>",
                         newfname);
        close(fd);
        unlink(newfname);
        return IMAP_IOERROR;
    }

    close(fd);

    /* rename the new header file over the old one */
    r = mailbox_meta_rename(mailbox, META_HEADER);
    if (r) return r;
    mailbox->header_dirty = 0; /* we wrote it out, so not dirty any more */

    /* re-read the header */
    r = mailbox_read_header(mailbox);
    if (r) return r;

    /* copy the new CRC into the index header */
    mailbox->i.header_file_crc = mailbox->header_file_crc;
    mailbox_index_dirty(mailbox);

    return 0;
}