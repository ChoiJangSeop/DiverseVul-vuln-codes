char* xread_file(const char *path) {
    FILE *fp = fopen(path, "r");
    char *result;
    size_t len;

    if (!fp)
        return NULL;

    result = fread_file_lim(fp, MAX_READ_LEN, &len);
    fclose (fp);

    if (result != NULL
        && len <= MAX_READ_LEN
        && (int) len == len)
        return result;

    free(result);
    return NULL;
}