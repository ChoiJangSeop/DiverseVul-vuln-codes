static void fuse_lo_data_cleanup(struct lo_data *lo)
{
    if (lo->inodes) {
        g_hash_table_destroy(lo->inodes);
    }

    if (lo->root.posix_locks) {
        g_hash_table_destroy(lo->root.posix_locks);
    }
    lo_map_destroy(&lo->fd_map);
    lo_map_destroy(&lo->dirp_map);
    lo_map_destroy(&lo->ino_map);

    if (lo->proc_self_fd >= 0) {
        close(lo->proc_self_fd);
    }

    if (lo->root.fd >= 0) {
        close(lo->root.fd);
    }

    free(lo->xattrmap);
    free_xattrmap(lo);
    free(lo->source);
}