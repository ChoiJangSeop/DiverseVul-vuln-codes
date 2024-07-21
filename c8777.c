void ProtocolV2::append_keepalive() {
  ldout(cct, 10) << __func__ << dendl;
  auto keepalive_frame = KeepAliveFrame::Encode();
  connection->outgoing_bl.append(keepalive_frame.get_buffer(session_stream_handlers));
}