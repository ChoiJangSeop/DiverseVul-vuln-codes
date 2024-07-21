int dd_chown(struct dump_dir *dd, uid_t new_uid)
{
    if (!dd->locked)
        error_msg_and_die("dump_dir is not opened"); /* bug */

    struct stat statbuf;
    if (!(stat(dd->dd_dirname, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)))
    {
        perror_msg("stat('%s')", dd->dd_dirname);
        return 1;
    }

    struct passwd *pw = getpwuid(new_uid);
    if (!pw)
    {
        error_msg("UID %ld is not found in user database", (long)new_uid);
        return 1;
    }

#if DUMP_DIR_OWNED_BY_USER > 0
    uid_t owners_uid = pw->pw_uid;
    gid_t groups_gid = statbuf.st_gid;
#else
    uid_t owners_uid = statbuf.st_uid;
    gid_t groups_gid = pw->pw_gid;
#endif

    int chown_res = lchown(dd->dd_dirname, owners_uid, groups_gid);
    if (chown_res)
        perror_msg("lchown('%s')", dd->dd_dirname);
    else
    {
        dd_init_next_file(dd);
        char *full_name;
        while (chown_res == 0 && dd_get_next_file(dd, /*short_name*/ NULL, &full_name))
        {
            log_debug("chowning %s", full_name);
            chown_res = lchown(full_name, owners_uid, groups_gid);
            if (chown_res)
                perror_msg("lchown('%s')", full_name);
            free(full_name);
        }
    }

    return chown_res;
}