check_user_password_V4(std::string const& user_password,
                       QPDF::EncryptionData const& data)
{
    // Algorithm 3.6 from the PDF 1.7 Reference Manual

    std::string u_value = compute_U_value(user_password, data);
    int to_compare = ((data.getR() >= 3) ? sizeof(MD5::Digest)
                      : key_bytes);
    return (memcmp(data.getU().c_str(), u_value.c_str(), to_compare) == 0);
}