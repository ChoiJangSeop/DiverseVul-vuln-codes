int diskutil_dd(const char *in, const char *out, const int bs, const long long count)
{
    char *output = NULL;

    if (in && out) {
        LOGINFO("copying data from '%s'\n", in);
        LOGINFO("               to '%s' (blocks=%lld)\n", out, count);
        output = pruntf(TRUE, "%s %s if=%s of=%s bs=%d count=%lld", helpers_path[ROOTWRAP], helpers_path[DD], in, out, bs, count);
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