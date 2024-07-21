void AES128GCM_OnWireTxHandler::reset_tx_handler(
  std::initializer_list<std::uint32_t> update_size_sequence)
{
  if(1 != EVP_EncryptInit_ex(ectx.get(), nullptr, nullptr, nullptr,
      reinterpret_cast<const unsigned char*>(&nonce))) {
    throw std::runtime_error("EVP_EncryptInit_ex failed");
  }

  buffer.reserve(std::accumulate(std::begin(update_size_sequence),
    std::end(update_size_sequence), AESGCM_TAG_LEN));

  ++nonce.random_seq;
}