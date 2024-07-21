bool Router::MatchView(const std::string& method, const std::string& url,
                       bool* stream) {
  assert(stream != nullptr);
  *stream = false;

  for (auto& route : routes_) {
    if (std::find(route.methods.begin(), route.methods.end(), method) ==
        route.methods.end()) {
      continue;
    }

    if (route.url.empty()) {
      std::smatch match;

      if (std::regex_match(url, match, route.url_regex)) {
        *stream = route.view->Stream(method);
        return true;
      }
    } else {
      if (boost::iequals(route.url, url)) {
        *stream = route.view->Stream(method);
        return true;
      }
    }
  }

  return false;
}