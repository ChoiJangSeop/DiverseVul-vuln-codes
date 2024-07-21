bool Decode(string_view encoded, std::string* raw) {
  for (auto iter = encoded.begin(); iter != encoded.end(); ++iter) {
    if (*iter == '%') {
      if (++iter == encoded.end()) {
        // Invalid URI string, two hexadecimal digits must follow '%'.
        return false;
      }

      int h_decimal = 0;
      if (!HexToDecimal(*iter, &h_decimal)) {
        return false;
      }

      if (++iter == encoded.end()) {
        // Invalid URI string, two hexadecimal digits must follow '%'.
        return false;
      }

      int l_decimal = 0;
      if (!HexToDecimal(*iter, &l_decimal)) {
        return false;
      }

      raw->push_back(static_cast<char>((h_decimal << 4) + l_decimal));

    } else if (*iter > 127 || *iter < 0) {
      // Invalid encoded URI string, must be entirely ASCII.
      return false;
    } else {
      raw->push_back(*iter);
    }
  }

  return true;
}