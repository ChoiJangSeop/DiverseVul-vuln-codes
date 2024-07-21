static int lo_do_open(struct lo_data *lo, struct lo_inode *inode,
                      int existing_fd, struct fuse_file_info *fi)
{
    ssize_t fh;
    int fd = existing_fd;
    int err;
    bool cap_fsetid_dropped = false;
    bool kill_suidgid = lo->killpriv_v2 && fi->kill_priv;

    update_open_flags(lo->writeback, lo->allow_direct_io, fi);

    if (fd < 0) {
        if (kill_suidgid) {
            err = drop_effective_cap("FSETID", &cap_fsetid_dropped);
            if (err) {
                return err;
            }
        }

        fd = lo_inode_open(lo, inode, fi->flags);

        if (cap_fsetid_dropped) {
            if (gain_effective_cap("FSETID")) {
                fuse_log(FUSE_LOG_ERR, "Failed to gain CAP_FSETID\n");
            }
        }
        if (fd < 0) {
            return -fd;
        }
    }

    pthread_mutex_lock(&lo->mutex);
    fh = lo_add_fd_mapping(lo, fd);
    pthread_mutex_unlock(&lo->mutex);
    if (fh == -1) {
        close(fd);
        return ENOMEM;
    }

    fi->fh = fh;
    if (lo->cache == CACHE_NONE) {
        fi->direct_io = 1;
    } else if (lo->cache == CACHE_ALWAYS) {
        fi->keep_cache = 1;
    }
    return 0;
}