void ConnectionImpl::releaseOutboundControlFrame(const Buffer::OwnedBufferFragmentImpl* fragment) {
  ASSERT(outbound_control_frames_ >= 1);
  --outbound_control_frames_;
  releaseOutboundFrame(fragment);
}