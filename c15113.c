pad_or_truncate_password_V4(std::string const& password)
{
    char k1[key_bytes];
    pad_or_truncate_password_V4(password, k1);
    return std::string(k1, key_bytes);
}