int dd_delete_item(struct dump_dir *dd, const char *name)
{
    if (!dd->locked)
        error_msg_and_die("dump_dir is not opened"); /* bug */

    if (!str_is_correct_filename(name))
        error_msg_and_die("Cannot delete item. '%s' is not a valid file name", name);

    char *path = concat_path_file(dd->dd_dirname, name);
    int res = unlink(path);

    if (res < 0)
    {
        if (errno == ENOENT)
            errno = res = 0;
        else
            perror_msg("Can't delete file '%s'", path);
    }

    free(path);
    return res;
}