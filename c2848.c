glob_(const char *pattern, int flags, int (*errfunc)(const char *, int),
      glob_t *pglob, unsigned long maxfiles, int maxdepth)
{
    const unsigned char *patnext;
    int c;
    Char *bufnext, *bufend, patbuf[PATH_MAX];
    struct glob_lim limit = { 0, 0, 0 };

    if (strlen(pattern) >= PATH_MAX) {
        return GLOB_NOMATCH;
    }
    pglob->gl_maxdepth = maxdepth;
    pglob->gl_maxfiles = maxfiles;
    patnext = (unsigned char *) pattern;
    if (!(flags & GLOB_APPEND)) {
        pglob->gl_pathc = 0;
        pglob->gl_pathv = NULL;
        pglob->gl_statv = NULL;
        if (!(flags & GLOB_DOOFFS)) {
            pglob->gl_offs = 0;
        }
    }
    pglob->gl_flags = flags & ~GLOB_MAGCHAR;
    pglob->gl_errfunc = errfunc;
    pglob->gl_matchc = 0;

    if (pglob->gl_offs < 0 || pglob->gl_pathc < 0 ||
        pglob->gl_offs >= INT_MAX || pglob->gl_pathc >= INT_MAX ||
        pglob->gl_pathc >= INT_MAX - pglob->gl_offs - 1) {
        return GLOB_NOSPACE;
    }
    bufnext = patbuf;
    bufend = bufnext + PATH_MAX - 1;
    if (flags & GLOB_NOESCAPE) {
        while (bufnext < bufend && (c = *patnext++) != EOS) {
            *bufnext++ = c;
        }
    } else {
        /* Protect the quoted characters. */
        while (bufnext < bufend && (c = *patnext++) != EOS) {
            if (c == QUOTE) {
                if ((c = *patnext++) == EOS) {
                    c = QUOTE;
                    --patnext;
                }
                *bufnext++ = c | M_PROTECT;
            } else {
                *bufnext++ = c;
            }
        }
    }
    *bufnext = EOS;

    if (flags & GLOB_BRACE) {
        return globexp1(patbuf, pglob, &limit, 0);
    } else {
        return glob0(patbuf, pglob, &limit);
    }
}