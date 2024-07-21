static void fuse_lib_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                             off_t off, struct fuse_file_info *llfi)
{
    struct fuse *f = req_fuse_prepare(req);
    struct fuse_file_info fi;
    struct fuse_dh *dh = get_dirhandle(llfi, &fi);

    pthread_mutex_lock(&dh->lock);
    /* According to SUS, directory contents need to be refreshed on
       rewinddir() */
    if (!off)
        dh->filled = 0;

    if (!dh->filled) {
        int err = readdir_fill(f, req, ino, size, off, dh, &fi);
        if (err) {
            reply_err(req, err);
            goto out;
        }
    }
    if (dh->filled) {
        if (off < dh->len) {
            if (off + size > dh->len)
                size = dh->len - off;
        } else
            size = 0;
    } else {
        size = dh->len;
        off = 0;
    }
    fuse_reply_buf(req, dh->contents + off, size);
 out:
    pthread_mutex_unlock(&dh->lock);
}