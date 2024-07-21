int dd_delete(struct dump_dir *dd)
{
    if (!dd->locked)
    {
        error_msg("unlocked problem directory %s cannot be deleted", dd->dd_dirname);
        return -1;
    }

    int r = delete_file_dir(dd->dd_dirname, /*skip_lock_file:*/ true);
    dd->locked = 0; /* delete_file_dir already removed .lock */
    dd_close(dd);
    return r;
}