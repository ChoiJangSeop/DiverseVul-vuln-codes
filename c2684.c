static int delete_file_dir(const char *dir, bool skip_lock_file)
{
    DIR *d = opendir(dir);
    if (!d)
    {
        /* The caller expects us to error out only if the directory
         * still exists (not deleted). If directory
         * *doesn't exist*, return 0 and clear errno.
         */
        if (errno == ENOENT || errno == ENOTDIR)
        {
            errno = 0;
            return 0;
        }
        return -1;
    }

    bool unlink_lock_file = false;
    struct dirent *dent;
    while ((dent = readdir(d)) != NULL)
    {
        if (dot_or_dotdot(dent->d_name))
            continue;
        if (skip_lock_file && strcmp(dent->d_name, ".lock") == 0)
        {
            unlink_lock_file = true;
            continue;
        }
        char *full_path = concat_path_file(dir, dent->d_name);
        if (unlink(full_path) == -1 && errno != ENOENT)
        {
            int err = 0;
            if (errno == EISDIR)
            {
                errno = 0;
                err = delete_file_dir(full_path, /*skip_lock_file:*/ false);
            }
            if (errno || err)
            {
                perror_msg("Can't remove '%s'", full_path);
                free(full_path);
                closedir(d);
                return -1;
            }
        }
        free(full_path);
    }
    closedir(d);

    /* Here we know for sure that all files/subdirs we found via readdir
     * were deleted successfully. If rmdir below fails, we assume someone
     * is racing with us and created a new file.
     */

    if (unlink_lock_file)
    {
        char *full_path = concat_path_file(dir, ".lock");
        xunlink(full_path);
        free(full_path);

        unsigned cnt = RMDIR_FAIL_COUNT;
        do {
            if (rmdir(dir) == 0)
                return 0;
            /* Someone locked the dir after unlink, but before rmdir.
             * This "someone" must be dd_lock().
             * It detects this (by seeing that there is no time file)
             * and backs off at once. So we need to just retry rmdir,
             * with minimal sleep.
             */
            usleep(RMDIR_FAIL_USLEEP);
        } while (--cnt != 0);
    }

    int r = rmdir(dir);
    if (r)
        perror_msg("Can't remove directory '%s'", dir);
    return r;
}