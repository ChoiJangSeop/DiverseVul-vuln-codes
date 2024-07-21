static void lo_write_buf(fuse_req_t req, fuse_ino_t ino,
                         struct fuse_bufvec *in_buf, off_t off,
                         struct fuse_file_info *fi)
{
    (void)ino;
    ssize_t res;
    struct fuse_bufvec out_buf = FUSE_BUFVEC_INIT(fuse_buf_size(in_buf));
    bool cap_fsetid_dropped = false;

    out_buf.buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
    out_buf.buf[0].fd = lo_fi_fd(req, fi);
    out_buf.buf[0].pos = off;

    fuse_log(FUSE_LOG_DEBUG,
             "lo_write_buf(ino=%" PRIu64 ", size=%zd, off=%lu kill_priv=%d)\n",
             ino, out_buf.buf[0].size, (unsigned long)off, fi->kill_priv);

    /*
     * If kill_priv is set, drop CAP_FSETID which should lead to kernel
     * clearing setuid/setgid on file. Note, for WRITE, we need to do
     * this even if killpriv_v2 is not enabled. fuse direct write path
     * relies on this.
     */
    if (fi->kill_priv) {
        res = drop_effective_cap("FSETID", &cap_fsetid_dropped);
        if (res != 0) {
            fuse_reply_err(req, res);
            return;
        }
    }

    res = fuse_buf_copy(&out_buf, in_buf);
    if (res < 0) {
        fuse_reply_err(req, -res);
    } else {
        fuse_reply_write(req, (size_t)res);
    }

    if (cap_fsetid_dropped) {
        res = gain_effective_cap("FSETID");
        if (res) {
            fuse_log(FUSE_LOG_ERR, "Failed to gain CAP_FSETID\n");
        }
    }
}