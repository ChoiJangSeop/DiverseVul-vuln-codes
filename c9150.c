void ConnectionImpl::releaseOutboundFrame(const Buffer::OwnedBufferFragmentImpl* fragment) {
  ASSERT(outbound_frames_ >= 1);
  --outbound_frames_;
  delete fragment;
}