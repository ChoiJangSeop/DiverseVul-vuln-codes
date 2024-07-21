std::string decodeBase64(
    const std::string& encoded) {

  if (encoded.size() == 0) {
    // special case, to prevent an integer overflow down below.
    return "";
  }

  using namespace boost::archive::iterators;
  using b64it =
    transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;

  std::string decoded = std::string(b64it(std::begin(encoded)),
                                    b64it(std::end(encoded)));

  uint32_t numPadding = std::count(encoded.begin(), encoded.end(), '=');
  decoded.erase(decoded.end() - numPadding, decoded.end());

  return decoded;
}