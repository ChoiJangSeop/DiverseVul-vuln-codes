void ConnectionImpl::onHeaderField(const char* data, size_t length) {
  // We previously already finished up the headers, these headers are
  // now trailers.
  if (header_parsing_state_ == HeaderParsingState::Done) {
    if (!enable_trailers_) {
      // Ignore trailers.
      return;
    }
    processing_trailers_ = true;
    header_parsing_state_ = HeaderParsingState::Field;
  }
  if (header_parsing_state_ == HeaderParsingState::Value) {
    completeLastHeader();
  }

  current_header_field_.append(data, length);
}