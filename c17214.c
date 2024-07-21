tparm_varargs(const char *string, ...)
{
    va_list ap;
    char *result;

    _nc_tparm_err = 0;
    va_start(ap, string);
#ifdef TRACE
    TPS(tname) = "tparm";
#endif /* TRACE */
    result = tparam_internal(TRUE, string, ap);
    va_end(ap);
    return result;
}