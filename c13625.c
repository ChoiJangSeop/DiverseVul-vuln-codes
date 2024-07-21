void BinaryProtocolReader::readStringBody(StrType& str, int32_t size) {
  checkStringSize(size);

  // Catch empty string case
  if (size == 0) {
    str.clear();
    return;
  }

  if (static_cast<int32_t>(in_.length()) < size) {
    str.reserve(size); // only reserve for multi iter case below
  }
  str.clear();
  size_t size_left = size;
  while (size_left > 0) {
    auto data = in_.peekBytes();
    auto data_avail = std::min(data.size(), size_left);
    if (data.empty()) {
      TProtocolException::throwExceededSizeLimit();
    }

    str.append((const char*)data.data(), data_avail);
    size_left -= data_avail;
    in_.skipNoAdvance(data_avail);
  }
}