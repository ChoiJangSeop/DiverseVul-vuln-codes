MD5::getDataChecksum(char const* buf, int len)
{
    MD5 m;
    m.encodeDataIncrementally(buf, len);
    return m.unparse();
}