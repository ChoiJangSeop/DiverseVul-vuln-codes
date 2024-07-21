int diskutil_dd2(const char *in, const char *out, const int bs, const long long count, const long long seek, const long long skip)
{
    char *output = NULL;

    if (in && out) {
        LOGINFO("copying data from '%s'\n", in);
        LOGINFO("               to '%s'\n", out);
        LOGINFO("               of %lld blocks (bs=%d), seeking %lld, skipping %lld\n", count, bs, seek, skip);
        output =
            pruntf(TRUE, "%s %s if=%s of=%s bs=%d count=%lld seek=%lld skip=%lld conv=notrunc,fsync", helpers_path[ROOTWRAP], helpers_path[DD], in, out, bs, count, seek, skip);
        if (!output) {
            LOGERROR("cannot copy '%s'\n", in);
            LOGERROR("                to '%s'\n", out);
            return (EUCA_ERROR);
        }

        EUCA_FREE(output);
        return (EUCA_OK);
    }

    LOGWARN("bad params: in=%s, out=%s\n", SP(in), SP(out));
    return (EUCA_INVALID_ERROR);
}