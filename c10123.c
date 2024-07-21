static void lo_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                       int valid, struct fuse_file_info *fi)
{
    int saverr;
    char procname[64];
    struct lo_data *lo = lo_data(req);
    struct lo_inode *inode;
    int ifd;
    int res;
    int fd = -1;

    inode = lo_inode(req, ino);
    if (!inode) {
        fuse_reply_err(req, EBADF);
        return;
    }

    ifd = inode->fd;

    /* If fi->fh is invalid we'll report EBADF later */
    if (fi) {
        fd = lo_fi_fd(req, fi);
    }

    if (valid & FUSE_SET_ATTR_MODE) {
        if (fi) {
            res = fchmod(fd, attr->st_mode);
        } else {
            sprintf(procname, "%i", ifd);
            res = fchmodat(lo->proc_self_fd, procname, attr->st_mode, 0);
        }
        if (res == -1) {
            saverr = errno;
            goto out_err;
        }
    }
    if (valid & (FUSE_SET_ATTR_UID | FUSE_SET_ATTR_GID)) {
        uid_t uid = (valid & FUSE_SET_ATTR_UID) ? attr->st_uid : (uid_t)-1;
        gid_t gid = (valid & FUSE_SET_ATTR_GID) ? attr->st_gid : (gid_t)-1;

        res = fchownat(ifd, "", uid, gid, AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW);
        if (res == -1) {
            saverr = errno;
            goto out_err;
        }
    }
    if (valid & FUSE_SET_ATTR_SIZE) {
        int truncfd;
        bool kill_suidgid;
        bool cap_fsetid_dropped = false;

        kill_suidgid = lo->killpriv_v2 && (valid & FUSE_SET_ATTR_KILL_SUIDGID);
        if (fi) {
            truncfd = fd;
        } else {
            truncfd = lo_inode_open(lo, inode, O_RDWR);
            if (truncfd < 0) {
                saverr = -truncfd;
                goto out_err;
            }
        }

        if (kill_suidgid) {
            res = drop_effective_cap("FSETID", &cap_fsetid_dropped);
            if (res != 0) {
                saverr = res;
                if (!fi) {
                    close(truncfd);
                }
                goto out_err;
            }
        }

        res = ftruncate(truncfd, attr->st_size);
        saverr = res == -1 ? errno : 0;

        if (cap_fsetid_dropped) {
            if (gain_effective_cap("FSETID")) {
                fuse_log(FUSE_LOG_ERR, "Failed to gain CAP_FSETID\n");
            }
        }
        if (!fi) {
            close(truncfd);
        }
        if (res == -1) {
            goto out_err;
        }
    }
    if (valid & (FUSE_SET_ATTR_ATIME | FUSE_SET_ATTR_MTIME)) {
        struct timespec tv[2];

        tv[0].tv_sec = 0;
        tv[1].tv_sec = 0;
        tv[0].tv_nsec = UTIME_OMIT;
        tv[1].tv_nsec = UTIME_OMIT;

        if (valid & FUSE_SET_ATTR_ATIME_NOW) {
            tv[0].tv_nsec = UTIME_NOW;
        } else if (valid & FUSE_SET_ATTR_ATIME) {
            tv[0] = attr->st_atim;
        }

        if (valid & FUSE_SET_ATTR_MTIME_NOW) {
            tv[1].tv_nsec = UTIME_NOW;
        } else if (valid & FUSE_SET_ATTR_MTIME) {
            tv[1] = attr->st_mtim;
        }

        if (fi) {
            res = futimens(fd, tv);
        } else {
            sprintf(procname, "%i", inode->fd);
            res = utimensat(lo->proc_self_fd, procname, tv, 0);
        }
        if (res == -1) {
            saverr = errno;
            goto out_err;
        }
    }
    lo_inode_put(lo, &inode);

    return lo_getattr(req, ino, fi);

out_err:
    lo_inode_put(lo, &inode);
    fuse_reply_err(req, saverr);
}