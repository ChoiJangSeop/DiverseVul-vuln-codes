static bool save_binary_file(const char *path, const char* data, unsigned size, uid_t uid, gid_t gid, mode_t mode)
{
    /* the mode is set by the caller, see dd_create() for security analysis */
    unlink(path);
    int fd = open(path, O_WRONLY | O_TRUNC | O_CREAT | O_NOFOLLOW, mode);
    if (fd < 0)
    {
        perror_msg("Can't open file '%s'", path);
        return false;
    }

    if (uid != (uid_t)-1L)
    {
        if (fchown(fd, uid, gid) == -1)
        {
            perror_msg("Can't change '%s' ownership to %lu:%lu", path, (long)uid, (long)gid);
        }
    }

    /* O_CREATE in the open() call above causes that the permissions of the
     * created file are (mode & ~umask)
     *
     * This is true only if we did create file. We are not sure we created it
     * in this case - it may exist already.
     */
    if (fchmod(fd, mode) == -1)
    {
        perror_msg("Can't change mode of '%s'", path);
    }

    unsigned r = full_write(fd, data, size);
    close(fd);
    if (r != size)
    {
        error_msg("Can't save file '%s'", path);
        return false;
    }

    return true;
}