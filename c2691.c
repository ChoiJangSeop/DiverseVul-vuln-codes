int dd_rename(struct dump_dir *dd, const char *new_path)
{
    if (!dd->locked)
    {
        error_msg("unlocked problem directory %s cannot be renamed", dd->dd_dirname);
        return -1;
    }

    int res = rename(dd->dd_dirname, new_path);
    if (res == 0)
    {
        free(dd->dd_dirname);
        dd->dd_dirname = rm_trailing_slashes(new_path);
    }
    return res;
}