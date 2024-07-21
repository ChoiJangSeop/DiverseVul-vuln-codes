bool Server::MatchViewOrStatic(const std::string& method,
                               const std::string& url, bool* stream) {
  if (Router::MatchView(method, url, stream)) {
    return true;
  }

  // Try to match a static file.
  if (method == methods::kGet && !doc_root_.empty()) {
    fs::path path = doc_root_ / url;

    fs::error_code ec;
    if (!fs::is_directory(path, ec) && fs::exists(path, ec)) {
      return true;
    }
  }

  return false;
}