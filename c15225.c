compute_O_rc4_key(std::string const& user_password,
		  std::string const& owner_password,
		  QPDF::EncryptionData const& data,
		  unsigned char key[OU_key_bytes_V4])
{
    if (data.getV() >= 5)
    {
	throw std::logic_error(
	    "compute_O_rc4_key called for file with V >= 5");
    }
    std::string password = owner_password;
    if (password.empty())
    {
	password = user_password;
    }
    MD5 md5;
    md5.encodeDataIncrementally(
	pad_or_truncate_password_V4(password).c_str(), key_bytes);
    MD5::Digest digest;
    int key_len = std::min(static_cast<int>(sizeof(digest)),
                           data.getLengthBytes());
    iterate_md5_digest(md5, digest, ((data.getR() >= 3) ? 50 : 0), key_len);
    memcpy(key, digest, OU_key_bytes_V4);
}