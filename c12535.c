UrlQuery::UrlQuery(const std::string& encoded_str) {
  if (!encoded_str.empty()) {
    // Split into key value pairs separated by '&'.
    for (std::size_t i = 0; i != std::string::npos;) {
      std::size_t j = encoded_str.find_first_of('&', i);

      std::string kv;
      if (j == std::string::npos) {
        kv = encoded_str.substr(i);
        i = std::string::npos;
      } else {
        kv = encoded_str.substr(i, j - i);
        i = j + 1;
      }

      string_view key;
      string_view value;
      if (SplitKV(kv, '=', false, &key, &value)) {
        parameters_.push_back({ DecodeUnsafe(key), DecodeUnsafe(value) });
      }
    }
  }
}