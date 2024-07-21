void DH_Server::build(SSL& ssl)
{
    DiffieHellman& dhServer = ssl.useCrypto().use_dh();

    int pSz, gSz, pubSz;
    dhServer.set_sizes(pSz, gSz, pubSz);
    dhServer.get_parms(parms_.alloc_p(pSz), parms_.alloc_g(gSz),
                       parms_.alloc_pub(pubSz));

    short sigSz = 0;
    mySTL::auto_ptr<Auth> auth;
    const CertManager& cert = ssl.getCrypto().get_certManager();
    
    if (ssl.getSecurity().get_parms().sig_algo_ == rsa_sa_algo) {
        if (cert.get_keyType() != rsa_sa_algo) {
            ssl.SetError(privateKey_error);
            return;
        }
        auth.reset(NEW_YS RSA(cert.get_privateKey(),
                   cert.get_privateKeyLength(), false));
    }
    else {
        if (cert.get_keyType() != dsa_sa_algo) {
            ssl.SetError(privateKey_error);
            return;
        }
        auth.reset(NEW_YS DSS(cert.get_privateKey(),
                   cert.get_privateKeyLength(), false));
        sigSz += DSS_ENCODED_EXTRA;
    }
    
    sigSz += auth->get_signatureLength();
    if (!sigSz) {
        ssl.SetError(privateKey_error);
        return;
    }

    length_ = 8; // pLen + gLen + YsLen + SigLen
    length_ += pSz + gSz + pubSz + sigSz;

    output_buffer tmp(length_);
    byte len[2];
    // P
    c16toa(pSz, len);
    tmp.write(len, sizeof(len));
    tmp.write(parms_.get_p(), pSz);
    // G
    c16toa(gSz, len);
    tmp.write(len, sizeof(len));
    tmp.write(parms_.get_g(), gSz);
    // Ys
    c16toa(pubSz, len);
    tmp.write(len, sizeof(len));
    tmp.write(parms_.get_pub(), pubSz);

    // Sig
    byte hash[FINISHED_SZ];
    MD5  md5;
    SHA  sha;
    signature_ = NEW_YS byte[sigSz];

    const Connection& conn = ssl.getSecurity().get_connection();
    // md5
    md5.update(conn.client_random_, RAN_LEN);
    md5.update(conn.server_random_, RAN_LEN);
    md5.update(tmp.get_buffer(), tmp.get_size());
    md5.get_digest(hash);

    // sha
    sha.update(conn.client_random_, RAN_LEN);
    sha.update(conn.server_random_, RAN_LEN);
    sha.update(tmp.get_buffer(), tmp.get_size());
    sha.get_digest(&hash[MD5_LEN]);

    if (ssl.getSecurity().get_parms().sig_algo_ == rsa_sa_algo)
        auth->sign(signature_, hash, sizeof(hash),
                   ssl.getCrypto().get_random());
    else {
        auth->sign(signature_, &hash[MD5_LEN], SHA_LEN,
                   ssl.getCrypto().get_random());
        byte encoded[DSS_SIG_SZ + DSS_ENCODED_EXTRA];
        TaoCrypt::EncodeDSA_Signature(signature_, encoded);
        memcpy(signature_, encoded, sizeof(encoded));
    }

    c16toa(sigSz, len);
    tmp.write(len, sizeof(len));
    tmp.write(signature_, sigSz);

    // key message
    keyMessage_ = NEW_YS opaque[length_];
    memcpy(keyMessage_, tmp.get_buffer(), tmp.get_size());
}