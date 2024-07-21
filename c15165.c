Pl_AES_PDF::initializeVector()
{
    if (use_zero_iv)
    {
	for (unsigned int i = 0; i < this->buf_size; ++i)
	{
	    this->cbc_block[i] = 0;
	}
    }
    else if (use_specified_iv)
    {
        std::memcpy(this->cbc_block, this->specified_iv, this->buf_size);
    }
    else if (use_static_iv)
    {
	for (unsigned int i = 0; i < this->buf_size; ++i)
	{
	    this->cbc_block[i] = 14 * (1 + i);
	}
    }
    else
    {
        QUtil::initializeWithRandomBytes(this->cbc_block, this->buf_size);
    }
}