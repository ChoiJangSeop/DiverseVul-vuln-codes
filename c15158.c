compute_Perms_value_V5_clear(std::string const& encryption_key,
                             QPDF::EncryptionData const& data,
                             unsigned char k[16])
{
    // From algorithm 3.10 from the PDF 1.7 extension level 3
    unsigned long long extended_perms = 0xffffffff00000000LL | data.getP();
    for (int i = 0; i < 8; ++i)
    {
        k[i] = static_cast<unsigned char>(extended_perms & 0xff);
        extended_perms >>= 8;
    }
    k[8] = data.getEncryptMetadata() ? 'T' : 'F';
    k[9] = 'a';
    k[10] = 'd';
    k[11] = 'b';
    QUtil::initializeWithRandomBytes(k + 12, 4);
}