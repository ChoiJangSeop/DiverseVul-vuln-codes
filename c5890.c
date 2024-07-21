static int path_set_perms(Item *i, const char *path) {
        char fn[STRLEN("/proc/self/fd/") + DECIMAL_STR_MAX(int)];
        _cleanup_close_ int fd = -1;
        struct stat st;

        assert(i);
        assert(path);

        if (!i->mode_set && !i->uid_set && !i->gid_set)
                goto shortcut;

        /* We open the file with O_PATH here, to make the operation
         * somewhat atomic. Also there's unfortunately no fchmodat()
         * with AT_SYMLINK_NOFOLLOW, hence we emulate it here via
         * O_PATH. */

        fd = open(path, O_NOFOLLOW|O_CLOEXEC|O_PATH);
        if (fd < 0) {
                int level = LOG_ERR, r = -errno;

                /* Option "e" operates only on existing objects. Do not
                 * print errors about non-existent files or directories */
                if (i->type == EMPTY_DIRECTORY && errno == ENOENT) {
                        level = LOG_DEBUG;
                        r = 0;
                }

                log_full_errno(level, errno, "Adjusting owner and mode for %s failed: %m", path);
                return r;
        }

        if (fstatat(fd, "", &st, AT_EMPTY_PATH) < 0)
                return log_error_errno(errno, "Failed to fstat() file %s: %m", path);

        xsprintf(fn, "/proc/self/fd/%i", fd);

        if (i->mode_set) {
                if (S_ISLNK(st.st_mode))
                        log_debug("Skipping mode fix for symlink %s.", path);
                else {
                        mode_t m = i->mode;

                        if (i->mask_perms) {
                                if (!(st.st_mode & 0111))
                                        m &= ~0111;
                                if (!(st.st_mode & 0222))
                                        m &= ~0222;
                                if (!(st.st_mode & 0444))
                                        m &= ~0444;
                                if (!S_ISDIR(st.st_mode))
                                        m &= ~07000; /* remove sticky/sgid/suid bit, unless directory */
                        }

                        if (m == (st.st_mode & 07777))
                                log_debug("\"%s\" has correct mode %o already.", path, st.st_mode);
                        else {
                                log_debug("Changing \"%s\" to mode %o.", path, m);

                                if (chmod(fn, m) < 0)
                                        return log_error_errno(errno, "chmod() of %s via %s failed: %m", path, fn);
                        }
                }
        }

        if ((i->uid_set && i->uid != st.st_uid) ||
            (i->gid_set && i->gid != st.st_gid)) {
                log_debug("Changing \"%s\" to owner "UID_FMT":"GID_FMT,
                          path,
                          i->uid_set ? i->uid : UID_INVALID,
                          i->gid_set ? i->gid : GID_INVALID);

                if (chown(fn,
                          i->uid_set ? i->uid : UID_INVALID,
                          i->gid_set ? i->gid : GID_INVALID) < 0)
                        return log_error_errno(errno, "chown() of %s via %s failed: %m", path, fn);
        }

        fd = safe_close(fd);

shortcut:
        return label_fix(path, false, false);
}