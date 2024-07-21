void CertificateVerify::Build(SSL& ssl)
{
    build_certHashes(ssl, hashes_);

    uint16 sz = 0;
    byte   len[VERIFY_HEADER];
    mySTL::auto_array<byte> sig;

    // sign
    const CertManager& cert = ssl.getCrypto().get_certManager();
    if (cert.get_keyType() == rsa_sa_algo) {
        RSA rsa(cert.get_privateKey(), cert.get_privateKeyLength(), false);

        sz = rsa.get_cipherLength() + VERIFY_HEADER;
        sig.reset(NEW_YS byte[sz]);

        c16toa(sz - VERIFY_HEADER, len);
        memcpy(sig.get(), len, VERIFY_HEADER);
        rsa.sign(sig.get() + VERIFY_HEADER, hashes_.md5_, sizeof(Hashes),
                 ssl.getCrypto().get_random());
    }
    else {  // DSA
        DSS dss(cert.get_privateKey(), cert.get_privateKeyLength(), false);

        sz = DSS_SIG_SZ + DSS_ENCODED_EXTRA + VERIFY_HEADER;
        sig.reset(NEW_YS byte[sz]);

        c16toa(sz - VERIFY_HEADER, len);
        memcpy(sig.get(), len, VERIFY_HEADER);
        dss.sign(sig.get() + VERIFY_HEADER, hashes_.sha_, SHA_LEN,
                 ssl.getCrypto().get_random());

        byte encoded[DSS_SIG_SZ + DSS_ENCODED_EXTRA];
        TaoCrypt::EncodeDSA_Signature(sig.get() + VERIFY_HEADER, encoded);
        memcpy(sig.get() + VERIFY_HEADER, encoded, sizeof(encoded));
    }
    set_length(sz);
    signature_ = sig.release();
}