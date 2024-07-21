int dd_get_next_file(struct dump_dir *dd, char **short_name, char **full_name)
{
    if (dd->next_dir == NULL)
        return 0;

    struct dirent *dent;
    while ((dent = readdir(dd->next_dir)) != NULL)
    {
        if (is_regular_file(dent, dd->dd_dirname))
        {
            if (short_name)
                *short_name = xstrdup(dent->d_name);
            if (full_name)
                *full_name = concat_path_file(dd->dd_dirname, dent->d_name);
            return 1;
        }
    }

    closedir(dd->next_dir);
    dd->next_dir = NULL;
    return 0;
}