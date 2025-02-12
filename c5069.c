static int local_name_to_path(FsContext *ctx, V9fsPath *dir_path,
                              const char *name, V9fsPath *target)
{
    if (dir_path) {
        v9fs_path_sprintf(target, "%s/%s", dir_path->data, name);
    } else {
        v9fs_path_sprintf(target, "%s", name);
    }
    return 0;
}