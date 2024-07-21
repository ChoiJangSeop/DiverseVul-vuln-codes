int dsa_test()
{
    Source source;
    FileSource("../certs/dsa1024.der", source);
    if (source.size() == 0) {
        FileSource("../../certs/dsa1024.der", source);  // for testsuite
        if (source.size() == 0) {
            FileSource("../../../certs/dsa1024.der", source); // win32 Debug dir
            if (source.size() == 0)
                err_sys("where's your certs dir?", -89);
        }
    }

    const char msg[] = "this is the message";
    byte signature[40];

    DSA_PrivateKey priv(source);
    DSA_Signer signer(priv);

    SHA sha;
    byte digest[SHA::DIGEST_SIZE];
    sha.Update((byte*)msg, sizeof(msg));
    sha.Final(digest);

    signer.Sign(digest, signature, rng);

    byte encoded[sizeof(signature) + 6];
    byte decoded[40];

    word32 encSz = EncodeDSA_Signature(signer.GetR(), signer.GetS(), encoded);
    DecodeDSA_Signature(decoded, encoded, encSz);

    DSA_PublicKey pub(priv);
    DSA_Verifier verifier(pub);

    if (!verifier.Verify(digest, decoded))
        return -90;

    return 0;
}