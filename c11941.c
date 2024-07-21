int diskutil_ddzero(const char *path, const long long sectors, boolean zero_fill)
{
    char *output = NULL;
    long long count = 1;
    long long seek = sectors - 1;

    if (path) {
        if (zero_fill) {
            count = sectors;
            seek = 0;
        }

        output = pruntf(TRUE, "%s %s if=/dev/zero of=%s bs=512 seek=%lld count=%lld", helpers_path[ROOTWRAP], helpers_path[DD], path, seek, count);
        if (!output) {
            LOGERROR("cannot create disk file %s\n", path);
            return (EUCA_ERROR);
        }

        EUCA_FREE(output);
        return (EUCA_OK);
    }

    LOGWARN("bad params: path=%s\n", SP(path));
    return (EUCA_INVALID_ERROR);
}