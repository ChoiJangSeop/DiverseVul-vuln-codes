int diskutil_ch(const char *path, const char *user, const char *group, const int perms)
{
    char *output = NULL;

    LOGDEBUG("ch(own|mod) '%s' %s.%s %o\n", SP(path), ((user != NULL) ? user : "*"), ((group != NULL) ? group : "*"), perms);

    if (path) {
        if (user) {
            output = pruntf(TRUE, "%s %s %s %s", helpers_path[ROOTWRAP], helpers_path[CHOWN], user, path);
            if (!output) {
                return (EUCA_ERROR);
            }
            EUCA_FREE(output);
        }

        if (group) {
            output = pruntf(TRUE, "%s %s :%s %s", helpers_path[ROOTWRAP], helpers_path[CHOWN], group, path);
            if (!output) {
                return (EUCA_ERROR);
            }
            EUCA_FREE(output);
        }

        if (perms > 0) {
            output = pruntf(TRUE, "%s %s 0%o %s", helpers_path[ROOTWRAP], helpers_path[CHMOD], perms, path);
            if (!output) {
                return (EUCA_ERROR);
            }
            EUCA_FREE(output);
        }

        return (EUCA_OK);
    }

    return (EUCA_INVALID_ERROR);
}