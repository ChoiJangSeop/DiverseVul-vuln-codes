char* xread_file(const char *path) {
    FILE *fp;
    char *result;

    fp = fopen(path, "r");
    result = xfread_file(fp);
    fclose (fp);

    return result;
}