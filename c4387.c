int SSL_CTX_load_verify_locations(SSL_CTX* ctx, const char* file,
                                  const char* path)
{
    int       ret = SSL_FAILURE;
    const int HALF_PATH = 128;

    if (file) ret = read_file(ctx, file, SSL_FILETYPE_PEM, CA);

    if (ret == SSL_SUCCESS && path) {
        // call read_file for each reqular file in path
#ifdef _WIN32

        WIN32_FIND_DATA FindFileData;
        HANDLE hFind;

        char name[MAX_PATH + 1];  // directory specification
        strncpy(name, path, MAX_PATH - 3);
        strncat(name, "\\*", 3);

        hFind = FindFirstFile(name, &FindFileData);
        if (hFind == INVALID_HANDLE_VALUE) return SSL_BAD_PATH;

        do {
            if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
                strncpy(name, path, MAX_PATH - 2 - HALF_PATH);
                strncat(name, "\\", 2);
                strncat(name, FindFileData.cFileName, HALF_PATH);
                ret = read_file(ctx, name, SSL_FILETYPE_PEM, CA);
            }
        } while (ret == SSL_SUCCESS && FindNextFile(hFind, &FindFileData));

        FindClose(hFind);

#else   // _WIN32

        const int MAX_PATH = 260;

        DIR* dir = opendir(path);
        if (!dir) return SSL_BAD_PATH;

        struct dirent* entry;
        struct stat    buf;
        char           name[MAX_PATH + 1];

        while (ret == SSL_SUCCESS && (entry = readdir(dir))) {
            strncpy(name, path, MAX_PATH - 1 - HALF_PATH);
            strncat(name, "/", 1);
            strncat(name, entry->d_name, HALF_PATH);
            if (stat(name, &buf) < 0) {
                closedir(dir);
                return SSL_BAD_STAT;
            }
     
            if (S_ISREG(buf.st_mode))
                ret = read_file(ctx, name, SSL_FILETYPE_PEM, CA);
        }

        closedir(dir);

#endif
    }

    return ret;
}