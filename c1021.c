struct dump_dir *dd_create(const char *dir, uid_t uid, mode_t mode)
{
    /* a little trick to copy read bits from file mode to exec bit of dir mode*/
    mode_t dir_mode = mode | ((mode & 0444) >> 2);
    struct dump_dir *dd = dd_init();

    dd->mode = mode;

    /* Unlike dd_opendir, can't use realpath: the directory doesn't exist yet,
     * realpath will always return NULL. We don't really have to:
     * dd_opendir(".") makes sense, dd_create(".") does not.
     */
    dir = dd->dd_dirname = rm_trailing_slashes(dir);

    const char *last_component = strrchr(dir, '/');
    if (last_component)
        last_component++;
    else
        last_component = dir;
    if (dot_or_dotdot(last_component))
    {
        /* dd_create("."), dd_create(".."), dd_create("dir/."),
         * dd_create("dir/..") and similar are madness, refuse them.
         */
        error_msg("Bad dir name '%s'", dir);
        dd_close(dd);
        return NULL;
    }

    /* Was creating it with mode 0700 and user as the owner, but this allows
     * the user to replace any file in the directory, changing security-sensitive data
     * (e.g. "uid", "analyzer", "executable")
     */
    if (g_mkdir_with_parents(dd->dd_dirname, dir_mode) != 0)
    {
        perror_msg("Can't create directory '%s'", dir);
        dd_close(dd);
        return NULL;
    }

    if (dd_lock(dd, CREATE_LOCK_USLEEP, /*flags:*/ 0) < 0)
    {
        dd_close(dd);
        return NULL;
    }

    /* mkdir's mode (above) can be affected by umask, fix it */
    if (chmod(dir, dir_mode) == -1)
    {
        perror_msg("Can't change mode of '%s'", dir);
        dd_close(dd);
        return NULL;
    }

    dd->dd_uid = (uid_t)-1L;
    dd->dd_gid = (gid_t)-1L;
    if (uid != (uid_t)-1L)
    {
        /* Get ABRT's user id */
        dd->dd_uid = 0;
        struct passwd *pw = getpwnam("abrt");
        if (pw)
            dd->dd_uid = pw->pw_uid;
        else
            error_msg("user 'abrt' does not exist, using uid 0");

        /* Get crashed application's group id */
        /*dd->dd_gid = 0; - dd_init did this already */
        pw = getpwuid(uid);
        if (pw)
            dd->dd_gid = pw->pw_gid;
        else
            error_msg("User %lu does not exist, using gid 0", (long)uid);

        if (chown(dir, dd->dd_uid, dd->dd_gid) == -1)
        {
            perror_msg("Can't change '%s' ownership to %lu:%lu", dir,
                       (long)dd->dd_uid, (long)dd->dd_gid);
        }
    }

    return dd;
}