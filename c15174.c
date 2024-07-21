void MD5::encodeString(char const* str)
{
    unsigned int len = strlen(str);

    update(QUtil::unsigned_char_pointer(str), len);
    final();
}