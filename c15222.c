void MD5::encodeDataIncrementally(char const* data, int len)
{
    update(QUtil::unsigned_char_pointer(data), len);
}