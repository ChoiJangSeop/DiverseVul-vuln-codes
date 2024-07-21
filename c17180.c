tiparm(const char *string, ...)
{
    va_list ap;
    char *result;

    _nc_tparm_err = 0;
    va_start(ap, string);
#ifdef TRACE
    TPS(tname) = "tiparm";
#endif /* TRACE */
    result = tparam_internal(FALSE, string, ap);
    va_end(ap);
    return result;
}