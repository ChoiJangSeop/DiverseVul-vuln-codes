void dd_save_binary(struct dump_dir* dd, const char* name, const char* data, unsigned size)
{
    if (!dd->locked)
        error_msg_and_die("dump_dir is not opened"); /* bug */

    char *full_path = concat_path_file(dd->dd_dirname, name);
    save_binary_file(full_path, data, size, dd->dd_uid, dd->dd_gid, dd->mode);
    free(full_path);
}