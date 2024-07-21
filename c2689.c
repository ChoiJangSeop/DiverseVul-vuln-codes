long dd_get_item_size(struct dump_dir *dd, const char *name)
{
    if (!str_is_correct_filename(name))
        error_msg_and_die("Cannot get item size. '%s' is not a valid file name", name);

    long size = -1;
    char *iname = concat_path_file(dd->dd_dirname, name);
    struct stat statbuf;

    if (lstat(iname, &statbuf) == 0 && S_ISREG(statbuf.st_mode))
        size = statbuf.st_size;
    else
    {
        if (errno == ENOENT)
            size = 0;
        else
            perror_msg("Can't get size of file '%s'", iname);
    }

    free(iname);

    return size;
}