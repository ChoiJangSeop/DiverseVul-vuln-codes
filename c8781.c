  AES128GCM_OnWireTxHandler(CephContext* const cct,
			    const key_t& key,
			    const nonce_t& nonce)
    : cct(cct),
      ectx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free),
      nonce(nonce) {
    ceph_assert_always(ectx);
    ceph_assert_always(key.size() * CHAR_BIT == 128);

    if (1 != EVP_EncryptInit_ex(ectx.get(), EVP_aes_128_gcm(),
			        nullptr, nullptr, nullptr)) {
      throw std::runtime_error("EVP_EncryptInit_ex failed");
    }

    if(1 != EVP_EncryptInit_ex(ectx.get(), nullptr, nullptr,
			       key.data(), nullptr)) {
      throw std::runtime_error("EVP_EncryptInit_ex failed");
    }
  }