int create_symlink_lockfile(const char* lock_file, const char* pid)
{
    while (symlink(pid, lock_file) != 0)
    {
        if (errno != EEXIST)
        {
            if (errno != ENOENT && errno != ENOTDIR && errno != EACCES)
            {
                perror_msg("Can't create lock file '%s'", lock_file);
                errno = 0;
            }
            return -1;
        }

        char pid_buf[sizeof(pid_t)*3 + 4];
        ssize_t r = readlink(lock_file, pid_buf, sizeof(pid_buf) - 1);
        if (r < 0)
        {
            if (errno == ENOENT)
            {
                /* Looks like lock_file was deleted */
                usleep(SYMLINK_RETRY_USLEEP); /* avoid CPU eating loop */
                continue;
            }
            perror_msg("Can't read lock file '%s'", lock_file);
            errno = 0;
            return -1;
        }
        pid_buf[r] = '\0';

        if (strcmp(pid_buf, pid) == 0)
        {
            log("Lock file '%s' is already locked by us", lock_file);
            return 0;
        }
        if (isdigit_str(pid_buf))
        {
            char pid_str[sizeof("/proc/") + sizeof(pid_buf)];
            snprintf(pid_str, sizeof(pid_str), "/proc/%s", pid_buf);
            if (access(pid_str, F_OK) == 0)
            {
                log("Lock file '%s' is locked by process %s", lock_file, pid_buf);
                return 0;
            }
            log("Lock file '%s' was locked by process %s, but it crashed?", lock_file, pid_buf);
        }
        /* The file may be deleted by now by other process. Ignore ENOENT */
        if (unlink(lock_file) != 0 && errno != ENOENT)
        {
            perror_msg("Can't remove stale lock file '%s'", lock_file);
            errno = 0;
            return -1;
        }
    }

    log_info("Locked '%s'", lock_file);
    return 1;
}