process_with_aes(std::string const& key,
                 bool encrypt,
                 std::string const& data,
                 size_t outlength = 0,
                 unsigned int repetitions = 1,
                 unsigned char const* iv = 0,
                 size_t iv_length = 0)
{
    Pl_Buffer buffer("buffer");
    Pl_AES_PDF aes("aes", &buffer, encrypt,
                   QUtil::unsigned_char_pointer(key),
                   key.length());
    if (iv)
    {
        aes.setIV(iv, iv_length);
    }
    else
    {
        aes.useZeroIV();
    }
    aes.disablePadding();
    for (unsigned int i = 0; i < repetitions; ++i)
    {
        aes.write(QUtil::unsigned_char_pointer(data), data.length());
    }
    aes.finish();
    PointerHolder<Buffer> bufp = buffer.getBuffer();
    if (outlength == 0)
    {
        outlength = bufp->getSize();
    }
    else
    {
        outlength = std::min(outlength, bufp->getSize());
    }
    return std::string(reinterpret_cast<char*>(bufp->getBuffer()), outlength);
}