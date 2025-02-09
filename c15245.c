compute_U_value_R2(std::string const& user_password,
		   QPDF::EncryptionData const& data)
{
    // Algorithm 3.4 from the PDF 1.7 Reference Manual

    std::string k1 = QPDF::compute_encryption_key(user_password, data);
    char udata[key_bytes];
    pad_or_truncate_password_V4("", udata);
    pad_short_parameter(k1, data.getLengthBytes());
    iterate_rc4(QUtil::unsigned_char_pointer(udata), key_bytes,
		QUtil::unsigned_char_pointer(k1),
                data.getLengthBytes(), 1, false);
    return std::string(udata, key_bytes);
}