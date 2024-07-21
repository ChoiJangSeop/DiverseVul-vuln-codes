static bool is_sst_request_valid(const std::string &msg) {
  size_t method_len = strlen(msg.c_str());

  if (method_len == 0) {
    return false;
  }

  std::string method = msg.substr(0, method_len);

  // Is this method allowed?
  auto res = std::find(std::begin(allowed_sst_methods),
                       std::end(allowed_sst_methods), method);
  if (res == std::end(allowed_sst_methods)) {
    return false;
  }

  const char *data_ptr = msg.c_str() + method_len + 1;
  size_t data_len = strlen(data_ptr);

  // method + null + data + null
  if (method_len + 1 + data_len + 1 != msg.length()) {
    // Someone tries to piggyback after 2nd null
    return false;
  }

  if (data_len > 0) {
    /* We allow custom sst scripts, so data can be anything.

      We could create and maintain the list of forbidden
      characters and the ways they could be used to inject the command to
      the OS. However this approach seems to be too error prone.
      Instead of this we will just allow alpha-num + a few special characters
      (colon, slash, dot, underscore, square brackets). */
    std::string data = msg.substr(method_len + 1, data_len);
    static const std::regex allowed_chars_regex("[\\w:\\/\\.\\[\\]]+");
    if (!std::regex_match(data, allowed_chars_regex)) {
      return false;
    }
  }
  return true;
}