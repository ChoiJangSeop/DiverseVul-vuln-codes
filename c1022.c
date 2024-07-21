static char *load_text_file(const char *path, unsigned flags)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        if (!(flags & DD_FAIL_QUIETLY_ENOENT))
            perror_msg("Can't open file '%s'", path);
        return (flags & DD_LOAD_TEXT_RETURN_NULL_ON_FAILURE ? NULL : xstrdup(""));
    }

    struct strbuf *buf_content = strbuf_new();
    int oneline = 0;
    int ch;
    while ((ch = fgetc(fp)) != EOF)
    {
//TODO? \r -> \n?
//TODO? strip trailing spaces/tabs?
        if (ch == '\n')
            oneline = (oneline << 1) | 1;
        if (ch == '\0')
            ch = ' ';
        if (isspace(ch) || ch >= ' ') /* used !iscntrl, but it failed on unicode */
            strbuf_append_char(buf_content, ch);
    }
    fclose(fp);

    char last = oneline != 0 ? buf_content->buf[buf_content->len - 1] : 0;
    if (last == '\n')
    {
        /* If file contains exactly one '\n' and it is at the end, remove it.
         * This enables users to use simple "echo blah >file" in order to create
         * short string items in dump dirs.
         */
        if (oneline == 1)
            buf_content->buf[--buf_content->len] = '\0';
    }
    else /* last != '\n' */
    {
        /* Last line is unterminated, fix it */
        /* Cases: */
        /* oneline=0: "qwe" - DONT fix this! */
        /* oneline=1: "qwe\nrty" - two lines in fact */
        /* oneline>1: "qwe\nrty\uio" */
        if (oneline >= 1)
            strbuf_append_char(buf_content, '\n');
    }

    return strbuf_free_nobuf(buf_content);
}