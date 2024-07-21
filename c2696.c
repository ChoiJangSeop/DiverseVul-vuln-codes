static void dd_unlock(struct dump_dir *dd)
{
    if (dd->locked)
    {
        dd->locked = 0;

        unsigned dirname_len = strlen(dd->dd_dirname);
        char lock_buf[dirname_len + sizeof("/.lock")];
        strcpy(lock_buf, dd->dd_dirname);
        strcpy(lock_buf + dirname_len, "/.lock");
        xunlink(lock_buf);

        log_info("Unlocked '%s'", lock_buf);
    }
}