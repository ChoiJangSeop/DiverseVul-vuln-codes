static int local_link(FsContext *ctx, V9fsPath *oldpath,
                      V9fsPath *dirpath, const char *name)
{
    char *odirpath = g_path_get_dirname(oldpath->data);
    char *oname = g_path_get_basename(oldpath->data);
    int ret = -1;
    int odirfd, ndirfd;

    odirfd = local_opendir_nofollow(ctx, odirpath);
    if (odirfd == -1) {
        goto out;
    }

    ndirfd = local_opendir_nofollow(ctx, dirpath->data);
    if (ndirfd == -1) {
        close_preserve_errno(odirfd);
        goto out;
    }

    ret = linkat(odirfd, oname, ndirfd, name, 0);
    if (ret < 0) {
        goto out_close;
    }

    /* now link the virtfs_metadata files */
    if (ctx->export_flags & V9FS_SM_MAPPED_FILE) {
        int omap_dirfd, nmap_dirfd;

        ret = mkdirat(ndirfd, VIRTFS_META_DIR, 0700);
        if (ret < 0 && errno != EEXIST) {
            goto err_undo_link;
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

        ret = linkat(omap_dirfd, oname, nmap_dirfd, name, 0);
        close_preserve_errno(nmap_dirfd);
        close_preserve_errno(omap_dirfd);
        if (ret < 0 && errno != ENOENT) {
            goto err_undo_link;
        }

        ret = 0;
    }
    goto out_close;

err:
    ret = -1;
err_undo_link:
    unlinkat_preserve_errno(ndirfd, name, 0);
out_close:
    close_preserve_errno(ndirfd);
    close_preserve_errno(odirfd);
out:
    g_free(oname);
    g_free(odirpath);
    return ret;
}