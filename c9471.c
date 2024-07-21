bool HeaderUtility::matchHeaders(const HeaderMap& request_headers, const HeaderData& header_data) {
  const HeaderEntry* header = request_headers.get(header_data.name_);

  if (header == nullptr) {
    return header_data.invert_match_ && header_data.header_match_type_ == HeaderMatchType::Present;
  }

  bool match;
  const absl::string_view header_view = header->value().getStringView();
  switch (header_data.header_match_type_) {
  case HeaderMatchType::Value:
    match = header_data.value_.empty() || header_view == header_data.value_;
    break;
  case HeaderMatchType::Regex:
    match = header_data.regex_->match(header_view);
    break;
  case HeaderMatchType::Range: {
    int64_t header_value = 0;
    match = absl::SimpleAtoi(header_view, &header_value) &&
            header_value >= header_data.range_.start() && header_value < header_data.range_.end();
    break;
  }
  case HeaderMatchType::Present:
    match = true;
    break;
  case HeaderMatchType::Prefix:
    match = absl::StartsWith(header_view, header_data.value_);
    break;
  case HeaderMatchType::Suffix:
    match = absl::EndsWith(header_view, header_data.value_);
    break;
  case HeaderMatchType::Contains:
    match = absl::StrContains(header_view, header_data.value_);
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  return match != header_data.invert_match_;
}