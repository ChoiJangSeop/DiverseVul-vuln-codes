static int dd_lock(struct dump_dir *dd, unsigned sleep_usec, int flags)
{
    if (dd->locked)
        error_msg_and_die("Locking bug on '%s'", dd->dd_dirname);

    char pid_buf[sizeof(long)*3 + 2];
    snprintf(pid_buf, sizeof(pid_buf), "%lu", (long)getpid());

    unsigned dirname_len = strlen(dd->dd_dirname);
    char lock_buf[dirname_len + sizeof("/.lock")];
    strcpy(lock_buf, dd->dd_dirname);
    strcpy(lock_buf + dirname_len, "/.lock");

    unsigned count = NO_TIME_FILE_COUNT;

 retry:
    while (1)
    {
        int r = create_symlink_lockfile(lock_buf, pid_buf);
        if (r < 0)
            return r; /* error */
        if (r > 0)
            break; /* locked successfully */
        /* Other process has the lock, wait for it to go away */
        usleep(sleep_usec);
    }

    /* Are we called by dd_opendir (as opposed to dd_create)? */
    if (sleep_usec == WAIT_FOR_OTHER_PROCESS_USLEEP) /* yes */
    {
        const char *missing_file = dd_check(dd);
        /* some of the required files don't exist. We managed to lock the directory
         * which was just created by somebody else, or is almost deleted
         * by delete_file_dir.
         * Unlock and back off.
         */
        if (missing_file)
        {
            xunlink(lock_buf);
            log_warning("Unlocked '%s' (no or corrupted '%s' file)", lock_buf, missing_file);
            if (--count == 0 || flags & DD_DONT_WAIT_FOR_LOCK)
            {
                errno = EISDIR; /* "this is an ordinary dir, not dump dir" */
                return -1;
            }
            usleep(NO_TIME_FILE_USLEEP);
            goto retry;
        }
    }

    dd->locked = true;
    return 0;
}