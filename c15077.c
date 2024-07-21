Pl_AES_PDF::Pl_AES_PDF(char const* identifier, Pipeline* next,
		       bool encrypt, unsigned char const* key,
                       unsigned int key_bytes) :
    Pipeline(identifier, next),
    encrypt(encrypt),
    cbc_mode(true),
    first(true),
    offset(0),
    nrounds(0),
    use_zero_iv(false),
    use_specified_iv(false),
    disable_padding(false)
{
    unsigned int keybits = 8 * key_bytes;
    assert(key_bytes == KEYLENGTH(keybits));
    this->key = new unsigned char[key_bytes];
    this->rk = new uint32_t[RKLENGTH(keybits)];
    unsigned int rk_bytes = RKLENGTH(keybits) * sizeof(uint32_t);
    std::memcpy(this->key, key, key_bytes);
    std::memset(this->rk, 0, rk_bytes);
    std::memset(this->inbuf, 0, this->buf_size);
    std::memset(this->outbuf, 0, this->buf_size);
    std::memset(this->cbc_block, 0, this->buf_size);
    if (encrypt)
    {
	this->nrounds = rijndaelSetupEncrypt(this->rk, this->key, keybits);
    }
    else
    {
	this->nrounds = rijndaelSetupDecrypt(this->rk, this->key, keybits);
    }
    assert(this->nrounds == NROUNDS(keybits));
}