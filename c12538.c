Server::Server(boost::asio::ip::tcp protocol, std::uint16_t port,
               const fs::path& doc_root)
    : protocol_(protocol),
      port_(port),
      doc_root_(doc_root),
      acceptor_(io_context_),
      signals_(io_context_) {
  AddSignals();
}