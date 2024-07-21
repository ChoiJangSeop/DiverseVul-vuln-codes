QPDF::compute_encryption_key_from_password(
    std::string const& password, EncryptionData const& data)
{
    // Algorithm 3.2 from the PDF 1.7 Reference Manual

    // This code does not properly handle Unicode passwords.
    // Passwords are supposed to be converted from OS codepage
    // characters to PDFDocEncoding.  Unicode passwords are supposed
    // to be converted to OS codepage before converting to
    // PDFDocEncoding.  We instead require the password to be
    // presented in its final form.

    MD5 md5;
    md5.encodeDataIncrementally(
	pad_or_truncate_password_V4(password).c_str(), key_bytes);
    md5.encodeDataIncrementally(data.getO().c_str(), key_bytes);
    char pbytes[4];
    int P = data.getP();
    pbytes[0] = static_cast<char>(P & 0xff);
    pbytes[1] = static_cast<char>((P >> 8) & 0xff);
    pbytes[2] = static_cast<char>((P >> 16) & 0xff);
    pbytes[3] = static_cast<char>((P >> 24) & 0xff);
    md5.encodeDataIncrementally(pbytes, 4);
    md5.encodeDataIncrementally(data.getId1().c_str(),
                                data.getId1().length());
    if ((data.getR() >= 4) && (! data.getEncryptMetadata()))
    {
	char bytes[4];
	memset(bytes, 0xff, 4);
	md5.encodeDataIncrementally(bytes, 4);
    }
    MD5::Digest digest;
    int key_len = std::min(static_cast<int>(sizeof(digest)),
                           data.getLengthBytes());
    iterate_md5_digest(md5, digest, ((data.getR() >= 3) ? 50 : 0), key_len);
    return std::string(reinterpret_cast<char*>(digest), key_len);
}