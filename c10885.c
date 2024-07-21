static int rm_rf_children_inner(
                int fd,
                const char *fname,
                int is_dir,
                RemoveFlags flags,
                const struct stat *root_dev) {

        struct stat st;
        int r, q = 0;

        assert(fd >= 0);
        assert(fname);

        if (is_dir < 0 ||
            root_dev ||
            (is_dir > 0 && (root_dev || (flags & REMOVE_SUBVOLUME)))) {

                r = fstatat_harder(fd, fname, &st, AT_SYMLINK_NOFOLLOW, flags);
                if (r < 0)
                        return r;

                is_dir = S_ISDIR(st.st_mode);
        }

        if (is_dir) {
                _cleanup_close_ int subdir_fd = -1;

                /* if root_dev is set, remove subdirectories only if device is same */
                if (root_dev && st.st_dev != root_dev->st_dev)
                        return 0;

                /* Stop at mount points */
                r = fd_is_mount_point(fd, fname, 0);
                if (r < 0)
                        return r;
                if (r > 0)
                        return 0;

                if ((flags & REMOVE_SUBVOLUME) && btrfs_might_be_subvol(&st)) {

                        /* This could be a subvolume, try to remove it */

                        r = btrfs_subvol_remove_fd(fd, fname, BTRFS_REMOVE_RECURSIVE|BTRFS_REMOVE_QUOTA);
                        if (r < 0) {
                                if (!IN_SET(r, -ENOTTY, -EINVAL))
                                        return r;

                                /* ENOTTY, then it wasn't a btrfs subvolume, continue below. */
                        } else
                                /* It was a subvolume, done. */
                                return 1;
                }

                subdir_fd = openat(fd, fname, O_RDONLY|O_NONBLOCK|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW|O_NOATIME);
                if (subdir_fd < 0)
                        return -errno;

                /* We pass REMOVE_PHYSICAL here, to avoid doing the fstatfs() to check the file system type
                 * again for each directory */
                q = rm_rf_children(TAKE_FD(subdir_fd), flags | REMOVE_PHYSICAL, root_dev);

        } else if (flags & REMOVE_ONLY_DIRECTORIES)
                return 0;

        r = unlinkat_harder(fd, fname, is_dir ? AT_REMOVEDIR : 0, flags);
        if (r < 0)
                return r;
        if (q < 0)
                return q;
        return 1;
}