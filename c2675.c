int is_regular_file(struct dirent *dent, const char *dirname)
{
    if (dent->d_type == DT_REG)
        return 1;
    if (dent->d_type != DT_UNKNOWN)
        return 0;

    char *fullname = xasprintf("%s/%s", dirname, dent->d_name);
    struct stat statbuf;
    int r = lstat(fullname, &statbuf);
    free(fullname);

    return r == 0 && S_ISREG(statbuf.st_mode);
}