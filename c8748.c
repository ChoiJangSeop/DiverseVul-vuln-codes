parsestr(struct compiling *c, const node *n, int *bytesmode, int *rawmode,
         PyObject **result, const char **fstr, Py_ssize_t *fstrlen)
{
    size_t len;
    const char *s = STR(n);
    int quote = Py_CHARMASK(*s);
    int fmode = 0;
    *bytesmode = 0;
    *rawmode = 0;
    *result = NULL;
    *fstr = NULL;
    if (Py_ISALPHA(quote)) {
        while (!*bytesmode || !*rawmode) {
            if (quote == 'b' || quote == 'B') {
                quote = *++s;
                *bytesmode = 1;
            }
            else if (quote == 'u' || quote == 'U') {
                quote = *++s;
            }
            else if (quote == 'r' || quote == 'R') {
                quote = *++s;
                *rawmode = 1;
            }
            else if (quote == 'f' || quote == 'F') {
                quote = *++s;
                fmode = 1;
            }
            else {
                break;
            }
        }
    }
    if (fmode && *bytesmode) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (quote != '\'' && quote != '\"') {
        PyErr_BadInternalCall();
        return -1;
    }
    /* Skip the leading quote char. */
    s++;
    len = strlen(s);
    if (len > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "string to parse is too long");
        return -1;
    }
    if (s[--len] != quote) {
        /* Last quote char must match the first. */
        PyErr_BadInternalCall();
        return -1;
    }
    if (len >= 4 && s[0] == quote && s[1] == quote) {
        /* A triple quoted string. We've already skipped one quote at
           the start and one at the end of the string. Now skip the
           two at the start. */
        s += 2;
        len -= 2;
        /* And check that the last two match. */
        if (s[--len] != quote || s[--len] != quote) {
            PyErr_BadInternalCall();
            return -1;
        }
    }

    if (fmode) {
        /* Just return the bytes. The caller will parse the resulting
           string. */
        *fstr = s;
        *fstrlen = len;
        return 0;
    }

    /* Not an f-string. */
    /* Avoid invoking escape decoding routines if possible. */
    *rawmode = *rawmode || strchr(s, '\\') == NULL;
    if (*bytesmode) {
        /* Disallow non-ASCII characters. */
        const char *ch;
        for (ch = s; *ch; ch++) {
            if (Py_CHARMASK(*ch) >= 0x80) {
                ast_error(c, n, "bytes can only contain ASCII "
                          "literal characters.");
                return -1;
            }
        }
        if (*rawmode)
            *result = PyBytes_FromStringAndSize(s, len);
        else
            *result = decode_bytes_with_escapes(c, n, s, len);
    } else {
        if (*rawmode)
            *result = PyUnicode_DecodeUTF8Stateful(s, len, NULL, NULL);
        else
            *result = decode_unicode_with_escapes(c, n, s, len);
    }
    return *result == NULL ? -1 : 0;
}