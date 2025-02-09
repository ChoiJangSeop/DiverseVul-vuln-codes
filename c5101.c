ssize_t v9fs_list_xattr(FsContext *ctx, const char *path,
                        void *value, size_t vsize)
{
    ssize_t size = 0;
    void *ovalue = value;
    XattrOperations *xops;
    char *orig_value, *orig_value_start;
    ssize_t xattr_len, parsed_len = 0, attr_len;
    char *dirpath, *name;
    int dirfd;

    /* Get the actual len */
    dirpath = g_path_get_dirname(path);
    dirfd = local_opendir_nofollow(ctx, dirpath);
    g_free(dirpath);
    if (dirfd == -1) {
        return -1;
    }

    name = g_path_get_basename(path);
    xattr_len = flistxattrat_nofollow(dirfd, name, value, 0);
    if (xattr_len <= 0) {
        g_free(name);
        close_preserve_errno(dirfd);
        return xattr_len;
    }

    /* Now fetch the xattr and find the actual size */
    orig_value = g_malloc(xattr_len);
    xattr_len = flistxattrat_nofollow(dirfd, name, orig_value, xattr_len);
    g_free(name);
    close_preserve_errno(dirfd);
    if (xattr_len < 0) {
        return -1;
    }

    /* store the orig pointer */
    orig_value_start = orig_value;
    while (xattr_len > parsed_len) {
        xops = get_xattr_operations(ctx->xops, orig_value);
        if (!xops) {
            goto next_entry;
        }

        if (!value) {
            size += xops->listxattr(ctx, path, orig_value, value, vsize);
        } else {
            size = xops->listxattr(ctx, path, orig_value, value, vsize);
            if (size < 0) {
                goto err_out;
            }
            value += size;
            vsize -= size;
        }
next_entry:
        /* Got the next entry */
        attr_len = strlen(orig_value) + 1;
        parsed_len += attr_len;
        orig_value += attr_len;
    }
    if (value) {
        size = value - ovalue;
    }

err_out:
    g_free(orig_value_start);
    return size;
}