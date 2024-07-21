bool wsrep_setup_allowed_sst_methods() {
  std::string methods(wsrep_sst_allowed_methods);

  /* Check if the string is valid. In method name we allow only:
     alpha-num
     underscore
     dash
     but w do not allow leading and trailing dash.
     Method names have to be separated by colons (spaces before/after colon are
     allowed). Below regex may seem difficult, but in fact it is not:

     1. Any number of leading spaces

       2. Followed by  alpha-num character or underscore
         3. Followed by any number of underscore/dash
         4. Followed by alpha-num character or underscore
         5. 3 - 4 group is optional
       6. Followed by colon (possibly with leading and/or trailing spaces)
       7. 2 - 6 group is optional

     8. Followed by alpha-num character or underscore
       9. Followed by any number of underscore/dash
       10. Followed by alpha-num character or underscore
       11. 9 -10 group is optional

     11. Followed by any number of trailing spaces */

  static std::regex validate_regex(
      "\\s*(\\w([_-]*\\w)*(\\s*,\\s*))*\\w([_-]*\\w)*\\s*", std::regex::nosubs);

  if (!std::regex_match(methods, validate_regex)) {
    WSREP_FATAL(
        "Wrong format of --wsrep-sst-allowed-methods parameter value: %s",
        wsrep_sst_allowed_methods);
    return true;
  }

  // split it into tokens and trim
  static std::regex split_regex("[^,\\s][^,]*[^,\\s]");
  for (auto it =
           std::sregex_iterator(methods.begin(), methods.end(), split_regex);
       it != std::sregex_iterator(); ++it) {
    allowed_sst_methods.push_back(it->str());
  }

  return false;
}