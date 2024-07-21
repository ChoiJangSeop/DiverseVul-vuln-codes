std::string DecodeUnsafe(string_view encoded) {
  std::string raw;
  if (Decode(encoded, &raw)) {
    return raw;
  }
  return ToString(encoded);
}