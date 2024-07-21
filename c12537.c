std::string WC2MB(const std::wstring& input, unsigned int code_page) {
  if (input.empty()) {
    return "";
  }

  // There do have other code pages which require the flags to be 0, e.g.,
  // 50220, 50211, and so on. But they are not included in our charset
  // dictionary. So, only consider 65001 (UTF-8) and 54936 (GB18030).
  DWORD flags = 0;
  if (code_page != 65001 && code_page != 54936) {
    flags = WC_NO_BEST_FIT_CHARS | WC_COMPOSITECHECK | WC_DEFAULTCHAR;
  }

  int length = ::WideCharToMultiByte(code_page, flags, &input[0],
                                     static_cast<int>(input.size()), NULL, 0,
                                     NULL, NULL);

  std::string output(length, '\0');

  ::WideCharToMultiByte(code_page, flags, &input[0],
                        static_cast<int>(input.size()), &output[0],
                        static_cast<int>(output.size()), NULL, NULL);

  return output;
}