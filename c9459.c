bool Url::initializeForConnect(GURL&& url) {
  // CONNECT requests can only contain "hostname:port"
  // https://github.com/nodejs/http-parser/blob/d9275da4650fd1133ddc96480df32a9efe4b059b/http_parser.c#L2503-L2506.
  if (!url.is_valid() || url.IsStandard()) {
    return false;
  }

  const auto& parsed = url.parsed_for_possibly_invalid_spec();
  // The parsed.scheme contains the URL's hostname (stored by GURL). While host and port have -1
  // as its length.
  if (parsed.scheme.len <= 0 || parsed.host.len > 0 || parsed.port.len > 0) {
    return false;
  }

  host_and_port_ = url.possibly_invalid_spec();
  const auto& parts = StringUtil::splitToken(host_and_port_, ":", /*keep_empty_string=*/true,
                                             /*trim_whitespace=*/false);
  if (parts.size() != 2 || static_cast<size_t>(parsed.scheme.len) != parts.at(0).size() ||
      !validPortForConnect(parts.at(1))) {
    return false;
  }

  return true;
}