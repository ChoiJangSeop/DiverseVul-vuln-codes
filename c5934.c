compute_O_value(std::string const& user_password,
		std::string const& owner_password,
		QPDF::EncryptionData const& data)
{
    // Algorithm 3.3 from the PDF 1.7 Reference Manual

    unsigned char O_key[OU_key_bytes_V4];
    compute_O_rc4_key(user_password, owner_password, data, O_key);

    char upass[key_bytes];
    pad_or_truncate_password_V4(user_password, upass);
    iterate_rc4(QUtil::unsigned_char_pointer(upass), key_bytes,
		O_key, data.getLengthBytes(),
                (data.getR() >= 3) ? 20 : 1, false);
    return std::string(upass, key_bytes);
}