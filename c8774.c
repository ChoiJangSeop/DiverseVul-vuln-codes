CtPtr ProtocolV2::write(const std::string &desc,
                        CONTINUATION_TYPE<ProtocolV2> &next,
                        F &frame) {
  ceph::bufferlist bl = frame.get_buffer(session_stream_handlers);
  return write(desc, next, bl);
}