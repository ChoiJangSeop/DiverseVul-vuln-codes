static bool exist_file_dir(const char *path)
{
    struct stat buf;
    if (stat(path, &buf) == 0)
    {
        if (S_ISDIR(buf.st_mode) || S_ISREG(buf.st_mode))
        {
            return true;
        }
    }
    return false;
}