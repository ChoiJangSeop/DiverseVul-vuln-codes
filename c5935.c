QUtil::string_to_ll(char const* str)
{
#ifdef _MSC_VER
    return _strtoi64(str, 0, 10);
#else
    return strtoll(str, 0, 10);
#endif
}