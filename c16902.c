inline int Http2Session::OnFrameReceive(nghttp2_session* handle,
                                        const nghttp2_frame* frame,
                                        void* user_data) {
  Http2Session* session = static_cast<Http2Session*>(user_data);
  session->statistics_.frame_count++;
  DEBUG_HTTP2SESSION2(session, "complete frame received: type: %d",
                      frame->hd.type);
  switch (frame->hd.type) {
    case NGHTTP2_DATA:
      session->HandleDataFrame(frame);
      break;
    case NGHTTP2_PUSH_PROMISE:
      // Intentional fall-through, handled just like headers frames
    case NGHTTP2_HEADERS:
      session->HandleHeadersFrame(frame);
      break;
    case NGHTTP2_SETTINGS:
      session->HandleSettingsFrame(frame);
      break;
    case NGHTTP2_PRIORITY:
      session->HandlePriorityFrame(frame);
      break;
    case NGHTTP2_GOAWAY:
      session->HandleGoawayFrame(frame);
      break;
    case NGHTTP2_PING:
      session->HandlePingFrame(frame);
    default:
      break;
  }
  return 0;
}