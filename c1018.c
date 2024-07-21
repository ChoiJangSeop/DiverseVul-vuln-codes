void dd_sanitize_mode_and_owner(struct dump_dir *dd)
{
    /* Don't sanitize if we aren't run under root:
     * we assume that during file creation (by whatever means,
     * even by "hostname >file" in abrt_event.conf)
     * normal umask-based mode setting takes care of correct mode,
     * and uid:gid is, of course, set to user's uid and gid.
     *
     * For root operating on /var/spool/abrt/USERS_PROBLEM, this isn't true:
     * "hostname >file", for example, would create file OWNED BY ROOT!
     * This routine resets mode and uid:gid for all such files.
     */
    if (dd->dd_uid == (uid_t)-1)
        return;

    if (!dd->locked)
        error_msg_and_die("dump_dir is not opened"); /* bug */

    DIR *d = opendir(dd->dd_dirname);
    if (!d)
        return;

    struct dirent *dent;
    while ((dent = readdir(d)) != NULL)
    {
        if (dent->d_name[0] == '.') /* ".lock", ".", ".."? skip */
            continue;
        char *full_path = concat_path_file(dd->dd_dirname, dent->d_name);
        struct stat statbuf;
        if (lstat(full_path, &statbuf) == 0 && S_ISREG(statbuf.st_mode))
        {
            if ((statbuf.st_mode & 0777) != dd->mode)
                chmod(full_path, dd->mode);
            if (statbuf.st_uid != dd->dd_uid || statbuf.st_gid != dd->dd_gid)
            {
                if (chown(full_path, dd->dd_uid, dd->dd_gid) != 0)
                {
                    perror_msg("Can't change '%s' ownership to %lu:%lu", full_path,
                               (long)dd->dd_uid, (long)dd->dd_gid);
                }
            }
        }
        free(full_path);
    }
    closedir(d);
}