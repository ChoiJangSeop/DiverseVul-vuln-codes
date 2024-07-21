struct dump_dir *dd_opendir(const char *dir, int flags)
{
    struct dump_dir *dd = dd_init();

    dir = dd->dd_dirname = rm_trailing_slashes(dir);

    struct stat stat_buf;
    if (stat(dir, &stat_buf) != 0)
        goto cant_access;
    /* & 0666 should remove the executable bit */
    dd->mode = (stat_buf.st_mode & 0666);

    errno = 0;
    if (dd_lock(dd, WAIT_FOR_OTHER_PROCESS_USLEEP, flags) < 0)
    {
        if ((flags & DD_OPEN_READONLY) && errno == EACCES)
        {
            /* Directory is not writable. If it seems to be readable,
             * return "read only" dd, not NULL */
            if (stat(dir, &stat_buf) == 0
             && S_ISDIR(stat_buf.st_mode)
             && access(dir, R_OK) == 0
            ) {
                if(dd_check(dd) != NULL)
                {
                    dd_close(dd);
                    dd = NULL;
                }
                return dd;
            }
        }
        if (errno == EISDIR)
        {
            /* EISDIR: dd_lock can lock the dir, but it sees no time file there,
             * even after it retried many times. It must be an ordinary directory!
             *
             * Without this check, e.g. abrt-action-print happily prints any current
             * directory when run without arguments, because its option -d DIR
             * defaults to "."!
             */
            error_msg("'%s' is not a problem directory", dir);
        }
        else
        {
 cant_access:
            if (errno == ENOENT || errno == ENOTDIR)
            {
                if (!(flags & DD_FAIL_QUIETLY_ENOENT))
                    error_msg("'%s' does not exist", dir);
            }
            else
            {
                if (!(flags & DD_FAIL_QUIETLY_EACCES))
                    perror_msg("Can't access '%s'", dir);
            }
        }
        dd_close(dd);
        return NULL;
    }

    dd->dd_uid = (uid_t)-1L;
    dd->dd_gid = (gid_t)-1L;
    if (geteuid() == 0)
    {
        /* In case caller would want to create more files, he'll need uid:gid */
        struct stat stat_buf;
        if (stat(dir, &stat_buf) != 0 || !S_ISDIR(stat_buf.st_mode))
        {
            error_msg("Can't stat '%s', or it is not a directory", dir);
            dd_close(dd);
            return NULL;
        }
        dd->dd_uid = stat_buf.st_uid;
        dd->dd_gid = stat_buf.st_gid;
    }

    return dd;
}