StringVal EncryptionFunctions::from_base64(FunctionContext* ctx, const StringVal& src) {
    if (src.len == 0 || src.is_null) {
        return StringVal::null();
    }

    int cipher_len = src.len;
    std::unique_ptr<char[]> p;
    p.reset(new char[cipher_len]);

    int ret_code = base64_decode((const char*)src.ptr, src.len, p.get());
    if (ret_code < 0) {
        return StringVal::null();
    }
    return AnyValUtil::from_buffer_temp(ctx, p.get(), ret_code);
}