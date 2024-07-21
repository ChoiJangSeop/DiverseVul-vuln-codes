std::string Utf16ToUtf8(const std::wstring& utf16_string) {
  return WC2MB(utf16_string, CP_UTF8);
}