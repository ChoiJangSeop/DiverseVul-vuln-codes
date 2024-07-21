Pl_AES_PDF::setIV(unsigned char const* iv, size_t bytes)
{
    if (bytes != this->buf_size)
    {
        throw std::logic_error(
            "Pl_AES_PDF: specified initialization vector"
            " size in bytes must be " + QUtil::int_to_string(bytes));
    }
    this->use_specified_iv = true;
    memcpy(this->specified_iv, iv, bytes);
}