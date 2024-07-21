std::string encodeBase64(const std::string& input) {
  using namespace boost::archive::iterators;
  using b64it = base64_from_binary<transform_width<const char*, 6, 8>>;

  auto data = input.data();
  std::string encoded(b64it(data), b64it(data + (input.length())));
  encoded.append((3 - (input.length() % 3)) % 3, '=');

  return encoded;
}