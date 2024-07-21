bool ConnectionImpl::addOutboundFrameFragment(Buffer::OwnedImpl& output, const uint8_t* data,
                                              size_t length) {
  // Reset the outbound frame type (set in the onBeforeFrameSend callback) since the
  // onBeforeFrameSend callback is not called for DATA frames.
  bool is_outbound_flood_monitored_control_frame = false;
  std::swap(is_outbound_flood_monitored_control_frame, is_outbound_flood_monitored_control_frame_);
  try {
    incrementOutboundFrameCount(is_outbound_flood_monitored_control_frame);
  } catch (const FrameFloodException&) {
    return false;
  }

  auto fragment = Buffer::OwnedBufferFragmentImpl::create(
      absl::string_view(reinterpret_cast<const char*>(data), length),
      is_outbound_flood_monitored_control_frame ? control_frame_buffer_releasor_
                                                : frame_buffer_releasor_);

  // The Buffer::OwnedBufferFragmentImpl object will be deleted in the *frame_buffer_releasor_
  // callback.
  output.addBufferFragment(*fragment.release());
  return true;
}