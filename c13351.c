StringVal decrypt(FunctionContext* ctx, const StringVal& src, const StringVal& key,
                  const StringVal& iv, EncryptionMode mode) {
    if (src.len == 0 || src.is_null) {
        return StringVal::null();
    }
    int cipher_len = src.len;
    std::unique_ptr<char[]> p;
    p.reset(new char[cipher_len]);
    int ret_code = 0;
    if (mode != AES_128_ECB && mode != AES_192_ECB && mode != AES_256_ECB && mode != AES_256_ECB &&
        mode != SM4_128_ECB) {
        if (iv.len == 0 || iv.is_null) {
            return StringVal::null();
        }
        int iv_len = 32; // max  key length 256 / 8
        std::unique_ptr<char[]> init_vec;
        init_vec.reset(new char[iv_len]);
        std::memset(init_vec.get(), 0, iv.len + 1);
        memcpy(init_vec.get(), iv.ptr, iv.len);
        ret_code = EncryptionUtil::decrypt(
                mode, (unsigned char*)src.ptr, src.len, (unsigned char*)key.ptr, key.len,
                (unsigned char*)init_vec.get(), true, (unsigned char*)p.get());
    } else {
        ret_code = EncryptionUtil::decrypt(mode, (unsigned char*)src.ptr, src.len,
                                           (unsigned char*)key.ptr, key.len, nullptr, true,
                                           (unsigned char*)p.get());
    }
    if (ret_code < 0) {
        return StringVal::null();
    }
    return AnyValUtil::from_buffer_temp(ctx, p.get(), ret_code);
}