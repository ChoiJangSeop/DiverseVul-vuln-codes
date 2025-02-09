static int local_renameat(FsContext *ctx, V9fsPath *olddir,
                          const char *old_name, V9fsPath *newdir,
                          const char *new_name)
{
    int ret;
    int odirfd, ndirfd;

    odirfd = local_opendir_nofollow(ctx, olddir->data);
    if (odirfd == -1) {
        return -1;
    }

    ndirfd = local_opendir_nofollow(ctx, newdir->data);
    if (ndirfd == -1) {
        close_preserve_errno(odirfd);
        return -1;
    }

    ret = renameat(odirfd, old_name, ndirfd, new_name);
    if (ret < 0) {
        goto out;
    }

    if (ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        int omap_dirfd, nmap_dirfd;

        ret = mkdirat(ndirfd, VIRTFS_META_DIR, 0700);
        if (ret < 0 && errno != EEXIST) {
            goto err_undo_rename;
        }

        omap_dirfd = openat_dir(odirfd, VIRTFS_META_DIR);
        if (omap_dirfd == -1) {
            goto err;
        }

        nmap_dirfd = openat_dir(ndirfd, VIRTFS_META_DIR);
        if (nmap_dirfd == -1) {
            close_preserve_errno(omap_dirfd);
            goto err;
        }

        /* rename the .virtfs_metadata files */
        ret = renameat(omap_dirfd, old_name, nmap_dirfd, new_name);
        close_preserve_errno(nmap_dirfd);
        close_preserve_errno(omap_dirfd);
        if (ret < 0 && errno != ENOENT) {
            goto err_undo_rename;
        }

        ret = 0;
    }
    goto out;

err:
    ret = -1;
err_undo_rename:
    renameat_preserve_errno(ndirfd, new_name, odirfd, old_name);
out:
    close_preserve_errno(ndirfd);
    close_preserve_errno(odirfd);
    return ret;
}