Filter::UpstreamRequest::~UpstreamRequest() {
  if (span_ != nullptr) {
    Tracing::HttpTracerUtility::finalizeUpstreamSpan(*span_, upstream_headers_.get(),
                                                     upstream_trailers_.get(), stream_info_,
                                                     Tracing::EgressConfig::get());
  }

  if (per_try_timeout_ != nullptr) {
    // Allows for testing.
    per_try_timeout_->disableTimer();
  }
  clearRequestEncoder();

  stream_info_.setUpstreamTiming(upstream_timing_);
  stream_info_.onRequestComplete();
  for (const auto& upstream_log : parent_.config_.upstream_logs_) {
    upstream_log->log(parent_.downstream_headers_, upstream_headers_.get(),
                      upstream_trailers_.get(), stream_info_);
  }
}