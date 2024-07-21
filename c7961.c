absl::optional<CelValue> RequestWrapper::operator[](CelValue key) const {
  if (!key.IsString()) {
    return {};
  }
  auto value = key.StringOrDie().value();

  if (value == Headers) {
    return CelValue::CreateMap(&headers_);
  } else if (value == Time) {
    return CelValue::CreateTimestamp(absl::FromChrono(info_.startTime()));
  } else if (value == Size) {
    // it is important to make a choice whether to rely on content-length vs stream info
    // (which is not available at the time of the request headers)
    if (headers_.value_ != nullptr && headers_.value_->ContentLength() != nullptr) {
      int64_t length;
      if (absl::SimpleAtoi(headers_.value_->ContentLength()->value().getStringView(), &length)) {
        return CelValue::CreateInt64(length);
      }
    } else {
      return CelValue::CreateInt64(info_.bytesReceived());
    }
  } else if (value == Duration) {
    auto duration = info_.requestComplete();
    if (duration.has_value()) {
      return CelValue::CreateDuration(absl::FromChrono(duration.value()));
    }
  }

  if (headers_.value_ != nullptr) {
    if (value == Path) {
      return convertHeaderEntry(headers_.value_->Path());
    } else if (value == UrlPath) {
      absl::string_view path = headers_.value_->Path()->value().getStringView();
      size_t query_offset = path.find('?');
      if (query_offset == absl::string_view::npos) {
        return CelValue::CreateString(path);
      }
      return CelValue::CreateString(path.substr(0, query_offset));
    } else if (value == Host) {
      return convertHeaderEntry(headers_.value_->Host());
    } else if (value == Scheme) {
      return convertHeaderEntry(headers_.value_->Scheme());
    } else if (value == Method) {
      return convertHeaderEntry(headers_.value_->Method());
    } else if (value == Referer) {
      return convertHeaderEntry(headers_.value_->Referer());
    } else if (value == ID) {
      return convertHeaderEntry(headers_.value_->RequestId());
    } else if (value == UserAgent) {
      return convertHeaderEntry(headers_.value_->UserAgent());
    } else if (value == TotalSize) {
      return CelValue::CreateInt64(info_.bytesReceived() + headers_.value_->byteSize());
    }
  }
  return {};
}