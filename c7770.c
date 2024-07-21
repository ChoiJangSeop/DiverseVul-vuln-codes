Downstream::Downstream(Upstream *upstream, MemchunkPool *mcpool,
                       int32_t stream_id)
    : dlnext(nullptr),
      dlprev(nullptr),
      response_sent_body_length(0),
      balloc_(1024, 1024),
      req_(balloc_),
      resp_(balloc_),
      request_start_time_(std::chrono::high_resolution_clock::now()),
      blocked_request_buf_(mcpool),
      request_buf_(mcpool),
      response_buf_(mcpool),
      upstream_(upstream),
      blocked_link_(nullptr),
      addr_(nullptr),
      num_retry_(0),
      stream_id_(stream_id),
      assoc_stream_id_(-1),
      downstream_stream_id_(-1),
      response_rst_stream_error_code_(NGHTTP2_NO_ERROR),
      affinity_cookie_(0),
      request_state_(DownstreamState::INITIAL),
      response_state_(DownstreamState::INITIAL),
      dispatch_state_(DispatchState::NONE),
      upgraded_(false),
      chunked_request_(false),
      chunked_response_(false),
      expect_final_response_(false),
      request_pending_(false),
      request_header_sent_(false),
      accesslog_written_(false),
      new_affinity_cookie_(false),
      blocked_request_data_eof_(false) {

  auto &timeoutconf = get_config()->http2.timeout;

  ev_timer_init(&upstream_rtimer_, &upstream_rtimeoutcb, 0.,
                timeoutconf.stream_read);
  ev_timer_init(&upstream_wtimer_, &upstream_wtimeoutcb, 0.,
                timeoutconf.stream_write);
  ev_timer_init(&downstream_rtimer_, &downstream_rtimeoutcb, 0.,
                timeoutconf.stream_read);
  ev_timer_init(&downstream_wtimer_, &downstream_wtimeoutcb, 0.,
                timeoutconf.stream_write);

  upstream_rtimer_.data = this;
  upstream_wtimer_.data = this;
  downstream_rtimer_.data = this;
  downstream_wtimer_.data = this;

  rcbufs_.reserve(32);
}