int dd_exist(const struct dump_dir *dd, const char *path)
{
    char *full_path = concat_path_file(dd->dd_dirname, path);
    int ret = exist_file_dir(full_path);
    free(full_path);
    return ret;
}