void ProtocolV2::append_keepalive_ack(utime_t &timestamp) {
  auto keepalive_ack_frame = KeepAliveFrameAck::Encode(timestamp);
  connection->outgoing_bl.append(keepalive_ack_frame.get_buffer(session_stream_handlers));
}