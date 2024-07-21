std::wstring Utf8ToUtf16(const std::string& utf8_string) {
  return MB2WC(utf8_string, CP_UTF8);
}