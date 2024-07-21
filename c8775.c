  ~AES128GCM_OnWireTxHandler() override {
    ::ceph::crypto::zeroize_for_security(&nonce, sizeof(nonce));
  }