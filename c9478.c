ExtractorImpl::extract(const Http::RequestHeaderMap& headers) const {
  std::vector<JwtLocationConstPtr> tokens;

  // Check header locations first
  for (const auto& location_it : header_locations_) {
    const auto& location_spec = location_it.second;
    ENVOY_LOG(debug, "extract {}", location_it.first);
    const Http::HeaderEntry* entry = headers.get(location_spec->header_);
    if (entry) {
      auto value_str = entry->value().getStringView();
      if (!location_spec->value_prefix_.empty()) {
        const auto pos = value_str.find(location_spec->value_prefix_);
        if (pos == absl::string_view::npos) {
          // value_prefix not found anywhere in value_str, so skip
          continue;
        }
        value_str = extractJWT(value_str, pos + location_spec->value_prefix_.length());
      }
      tokens.push_back(std::make_unique<const JwtHeaderLocation>(
          std::string(value_str), location_spec->specified_issuers_, location_spec->header_));
    }
  }

  // If no query parameter locations specified, or Path() is null, bail out
  if (param_locations_.empty() || headers.Path() == nullptr) {
    return tokens;
  }

  // Check query parameter locations.
  const auto& params = Http::Utility::parseAndDecodeQueryString(headers.getPathValue());
  for (const auto& location_it : param_locations_) {
    const auto& param_key = location_it.first;
    const auto& location_spec = location_it.second;
    const auto& it = params.find(param_key);
    if (it != params.end()) {
      tokens.push_back(std::make_unique<const JwtParamLocation>(
          it->second, location_spec.specified_issuers_, param_key));
    }
  }
  return tokens;
}