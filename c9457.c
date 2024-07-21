bool Url::validPortForConnect(absl::string_view port_string) {
  int port;
  const bool valid = absl::SimpleAtoi(port_string, &port);
  // Only a port value in valid range (1-65535) is allowed.
  if (!valid || port <= 0 || port > std::numeric_limits<uint16_t>::max()) {
    return false;
  }
  port_ = static_cast<uint16_t>(port);
  return true;
}