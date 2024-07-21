std::wstring MB2WC(const std::string& input, unsigned int code_page) {
  if (input.empty()) {
    return L"";
  }

  int length = ::MultiByteToWideChar(code_page, 0, &input[0],
                                     static_cast<int>(input.size()),
                                     NULL, 0);

  std::wstring output(length, '\0');

  ::MultiByteToWideChar(code_page, 0, &input[0], static_cast<int>(input.size()),
                        &output[0], static_cast<int>(output.size()));

  return output;
}