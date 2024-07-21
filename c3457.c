int CertManager::Validate()
{
    CertList::reverse_iterator last = peerList_.rbegin();
    size_t count = peerList_.size();

    while ( count > 1 ) {
        TaoCrypt::Source source((*last)->get_buffer(), (*last)->get_length());
        TaoCrypt::CertDecoder cert(source, true, &signers_, verifyNone_);

        if (int err = cert.GetError().What())
            return err;

        const TaoCrypt::PublicKey& key = cert.GetPublicKey();
        signers_.push_back(NEW_YS TaoCrypt::Signer(key.GetKey(), key.size(),
                                        cert.GetCommonName(), cert.GetHash()));
        ++last;
        --count;
    }

    if (count) {
        // peer's is at the front
        TaoCrypt::Source source((*last)->get_buffer(), (*last)->get_length());
        TaoCrypt::CertDecoder cert(source, true, &signers_, verifyNone_);

        int err = cert.GetError().What();
        if ( err && err != TaoCrypt::SIG_OTHER_E)
            return err;

        uint sz = cert.GetPublicKey().size();
        peerPublicKey_.allocate(sz);
        peerPublicKey_.assign(cert.GetPublicKey().GetKey(), sz);

        if (cert.GetKeyType() == TaoCrypt::RSAk)
            peerKeyType_ = rsa_sa_algo;
        else
            peerKeyType_ = dsa_sa_algo;

        size_t iSz = strlen(cert.GetIssuer()) + 1;
        size_t sSz = strlen(cert.GetCommonName()) + 1;
        int bSz = (int)strlen(cert.GetBeforeDate()) + 1;
        int aSz = (int)strlen(cert.GetAfterDate()) + 1;
        peerX509_ = NEW_YS X509(cert.GetIssuer(), iSz, cert.GetCommonName(),
                                sSz, cert.GetBeforeDate(), bSz,
                                cert.GetAfterDate(), aSz);

        if (err == TaoCrypt::SIG_OTHER_E && verifyCallback_) {
            X509_STORE_CTX store;
            store.error = err;
            store.error_depth = static_cast<int>(count) - 1;
            store.current_cert = peerX509_;

            int ok = verifyCallback_(0, &store);
            if (ok) return 0;
        }

        if (err == TaoCrypt::SIG_OTHER_E) return err;
    }
    return 0;
}