StringVal EncryptionFunctions::to_base64(FunctionContext* ctx, const StringVal& src) {
    if (src.len == 0 || src.is_null) {
        return StringVal::null();
    }

    int cipher_len = (size_t)(4.0 * ceil((double)src.len / 3.0));
    std::unique_ptr<char[]> p;
    p.reset(new char[cipher_len]);

    int ret_code = base64_encode((unsigned char*)src.ptr, src.len, (unsigned char*)p.get());
    if (ret_code < 0) {
        return StringVal::null();
    }
    return AnyValUtil::from_buffer_temp(ctx, p.get(), ret_code);
}