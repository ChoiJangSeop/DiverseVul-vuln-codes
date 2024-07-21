void ActiveStreamEncoderFilter::responseDataTooLarge() {
  if (parent_.state_.encoder_filters_streaming_) {
    onEncoderFilterAboveWriteBufferHighWatermark();
  } else {
    parent_.filter_manager_callbacks_.onResponseDataTooLarge();

    // In this case, sendLocalReply will either send a response directly to the encoder, or
    // reset the stream.
    parent_.sendLocalReply(
        Http::Code::InternalServerError, CodeUtility::toString(Http::Code::InternalServerError),
        nullptr, absl::nullopt, StreamInfo::ResponseCodeDetails::get().ResponsePayloadTooLarge);
  }
}