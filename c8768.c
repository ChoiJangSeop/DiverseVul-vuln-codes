static int PARSER_FLAGS(PyCompilerFlags *flags)
{
    int parser_flags = 0;
    if (!flags)
        return 0;
    if (flags->cf_flags & PyCF_DONT_IMPLY_DEDENT)
        parser_flags |= PyPARSE_DONT_IMPLY_DEDENT;
    if (flags->cf_flags & PyCF_IGNORE_COOKIE)
        parser_flags |= PyPARSE_IGNORE_COOKIE;
    if (flags->cf_flags & CO_FUTURE_BARRY_AS_BDFL)
        parser_flags |= PyPARSE_BARRY_AS_BDFL;
    return parser_flags;
}