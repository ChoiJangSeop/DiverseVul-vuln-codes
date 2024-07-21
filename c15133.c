pad_or_truncate_password_V4(std::string const& password, char k1[key_bytes])
{
    int password_bytes = std::min(static_cast<size_t>(key_bytes),
                                  password.length());
    int pad_bytes = key_bytes - password_bytes;
    memcpy(k1, password.c_str(), password_bytes);
    memcpy(k1 + password_bytes, padding_string, pad_bytes);
}