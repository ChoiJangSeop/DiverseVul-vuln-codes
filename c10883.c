int unlinkat_harder(int dfd, const char *filename, int unlink_flags, RemoveFlags remove_flags) {

        mode_t old_mode;
        int r;

        /* Like unlinkat(), but tries harder: if we get EACCESS we'll try to set the r/w/x bits on the
         * directory. This is useful if we run unprivileged and have some files where the w bit is
         * missing. */

        if (unlinkat(dfd, filename, unlink_flags) >= 0)
                return 0;
        if (errno != EACCES || !FLAGS_SET(remove_flags, REMOVE_CHMOD))
                return -errno;

        r = patch_dirfd_mode(dfd, &old_mode);
        if (r < 0)
                return r;

        if (unlinkat(dfd, filename, unlink_flags) < 0) {
                r = -errno;
                /* Try to restore the original access mode if this didn't work */
                (void) fchmod(dfd, old_mode);
                return r;
        }

        if (FLAGS_SET(remove_flags, REMOVE_CHMOD_RESTORE) && fchmod(dfd, old_mode) < 0)
                return -errno;

        /* If this worked, we won't reset the old mode by default, since we'll need it for other entries too,
         * and we should destroy the whole thing */
        return 0;
}