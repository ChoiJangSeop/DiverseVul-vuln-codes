void AES128GCM_OnWireRxHandler::reset_rx_handler()
{
  if(1 != EVP_DecryptInit_ex(ectx.get(), nullptr, nullptr, nullptr,
	reinterpret_cast<const unsigned char*>(&nonce))) {
    throw std::runtime_error("EVP_DecryptInit_ex failed");
  }
  ++nonce.random_seq;
}