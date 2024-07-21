void dd_save_text(struct dump_dir *dd, const char *name, const char *data)
{
    if (!dd->locked)
        error_msg_and_die("dump_dir is not opened"); /* bug */

    char *full_path = concat_path_file(dd->dd_dirname, name);
    save_binary_file(full_path, data, strlen(data), dd->dd_uid, dd->dd_gid, dd->mode);
    free(full_path);
}