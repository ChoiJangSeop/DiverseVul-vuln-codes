void ConnectionImpl::onHeaderValue(const char* data, size_t length) {
  if (header_parsing_state_ == HeaderParsingState::Done && !enable_trailers_) {
    // Ignore trailers.
    return;
  }

  if (processing_trailers_) {
    maybeAllocTrailers();
  }

  absl::string_view header_value{data, length};

  if (strict_header_validation_) {
    if (!Http::HeaderUtility::headerValueIsValid(header_value)) {
      ENVOY_CONN_LOG(debug, "invalid header value: {}", connection_, header_value);
      error_code_ = Http::Code::BadRequest;
      sendProtocolError(Http1ResponseCodeDetails::get().InvalidCharacters);
      throw CodecProtocolException("http/1.1 protocol error: header value contains invalid chars");
    }
  }

  header_parsing_state_ = HeaderParsingState::Value;
  if (current_header_value_.empty()) {
    // Strip leading whitespace if the current header value input contains the first bytes of the
    // encoded header value. Trailing whitespace is stripped once the full header value is known in
    // ConnectionImpl::completeLastHeader. http_parser does not strip leading or trailing whitespace
    // as the spec requires: https://tools.ietf.org/html/rfc7230#section-3.2.4 .
    header_value = StringUtil::ltrim(header_value);
  }
  current_header_value_.append(header_value.data(), header_value.length());

  const uint32_t total =
      current_header_field_.size() + current_header_value_.size() + headersOrTrailers().byteSize();
  if (total > (max_headers_kb_ * 1024)) {
    const absl::string_view header_type =
        processing_trailers_ ? Http1HeaderTypes::get().Trailers : Http1HeaderTypes::get().Headers;
    error_code_ = Http::Code::RequestHeaderFieldsTooLarge;
    sendProtocolError(Http1ResponseCodeDetails::get().HeadersTooLarge);
    throw CodecProtocolException(absl::StrCat(header_type, " size exceeds limit"));
  }
}