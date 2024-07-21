static char* is_text_file(const char *name, ssize_t *sz)
{
    /* We were using magic.h API to check for file being text, but it thinks
     * that file containing just "0" is not text (!!)
     * So, we do it ourself.
     */

    int fd = open(name, O_RDONLY);
    if (fd < 0)
        return NULL; /* it's not text (because it does not exist! :) */

    off_t size = lseek(fd, 0, SEEK_END);
    if (size < 0)
    {
        close(fd);
        return NULL; /* it's not text (because there is an I/O error) */
    }
    lseek(fd, 0, SEEK_SET);

    unsigned char *buf = xmalloc(*sz);
    ssize_t r = full_read(fd, buf, *sz);
    close(fd);
    if (r < 0)
    {
        free(buf);
        return NULL; /* it's not text (because we can't read it) */
    }
    if (r < *sz)
        buf[r] = '\0';
    *sz = r;

    /* Some files in our dump directories are known to always be textual */
    const char *base = strrchr(name, '/');
    if (base)
    {
        base++;
        if (is_in_string_list(base, (char**)always_text_files))
            goto text;
    }

    /* Every once in a while, even a text file contains a few garbled
     * or unexpected non-ASCII chars. We should not declare it "binary".
     *
     * Used to have RATIO = 50 (2%), but then came Fedora 19 with
     * os_release = "Schrödinger's Cat". Bumped to 10%.
     * Alternatives: add os_release to always_text_files[]
     * or add "if it is valid Unicode, then it's text" check here.
     *
     * Replaced crude "buf[r] > 0x7e is bad" logic with
     * "if it is a broken Unicode, then it's bad".
     */
    const unsigned RATIO = 10;
    unsigned total_chars = r + RATIO;
    unsigned bad_chars = 1; /* 1 prevents division by 0 later */
    bool prev_was_unicode = 0;
    ssize_t i = -1;
    while (++i < r)
    {
        /* Among control chars, only '\t','\n' etc are allowed */
        if (buf[i] < ' ' && !isspace(buf[i]))
        {
            /* We don't like NULs and other control chars very much.
             * Not text for sure!
             */
            free(buf);
            return NULL;
        }
        if (buf[i] == 0x7f)
            bad_chars++;
        else if (buf[i] > 0x7f)
        {
            /* We test two possible bad cases with one comparison:
             * (1) prev byte was unicode AND cur byte is 11xxxxxx:
             * BAD - unicode start byte can't be in the middle of unicode char
             * (2) prev byte wasnt unicode AND cur byte is 10xxxxxx:
             * BAD - unicode continuation byte can't start unicode char
             */
            if (prev_was_unicode == ((buf[i] & 0x40) == 0x40))
                bad_chars++;
        }
        prev_was_unicode = (buf[i] > 0x7f);
    }

    if ((total_chars / bad_chars) >= RATIO)
        goto text; /* looks like text to me */

    free(buf);
    return NULL; /* it's binary */

 text:
    if (size > CD_MAX_TEXT_SIZE)
    {
        free(buf);
        return HUGE_TEXT;
    }
    return (char*)buf;
}