void Initialize(Local<Object> target,
                Local<Value> unused,
                Local<Context> context,
                void* priv) {
  Environment* env = Environment::GetCurrent(context);
  Isolate* isolate = env->isolate();
  HandleScope scope(isolate);

  std::unique_ptr<http2_state> state(new http2_state(isolate));

#define SET_STATE_TYPEDARRAY(name, field)             \
  target->Set(context,                                \
              FIXED_ONE_BYTE_STRING(isolate, (name)), \
              (field)).FromJust()

  // Initialize the buffer used for padding callbacks
  SET_STATE_TYPEDARRAY(
    "paddingBuffer", state->padding_buffer.GetJSArray());
  // Initialize the buffer used to store the session state
  SET_STATE_TYPEDARRAY(
    "sessionState", state->session_state_buffer.GetJSArray());
  // Initialize the buffer used to store the stream state
  SET_STATE_TYPEDARRAY(
    "streamState", state->stream_state_buffer.GetJSArray());
  SET_STATE_TYPEDARRAY(
    "settingsBuffer", state->settings_buffer.GetJSArray());
  SET_STATE_TYPEDARRAY(
    "optionsBuffer", state->options_buffer.GetJSArray());
#undef SET_STATE_TYPEDARRAY

  env->set_http2_state(std::move(state));

  NODE_DEFINE_CONSTANT(target, PADDING_BUF_FRAME_LENGTH);
  NODE_DEFINE_CONSTANT(target, PADDING_BUF_MAX_PAYLOAD_LENGTH);
  NODE_DEFINE_CONSTANT(target, PADDING_BUF_RETURN_VALUE);

  // Method to fetch the nghttp2 string description of an nghttp2 error code
  env->SetMethod(target, "nghttp2ErrorString", HttpErrorString);

  Local<String> http2SessionClassName =
    FIXED_ONE_BYTE_STRING(isolate, "Http2Session");

  Local<FunctionTemplate> ping = FunctionTemplate::New(env->isolate());
  ping->SetClassName(FIXED_ONE_BYTE_STRING(env->isolate(), "Http2Ping"));
  AsyncWrap::AddWrapMethods(env, ping);
  Local<ObjectTemplate> pingt = ping->InstanceTemplate();
  pingt->SetInternalFieldCount(1);
  env->set_http2ping_constructor_template(pingt);

  Local<FunctionTemplate> setting = FunctionTemplate::New(env->isolate());
  setting->SetClassName(FIXED_ONE_BYTE_STRING(env->isolate(), "Http2Setting"));
  AsyncWrap::AddWrapMethods(env, setting);
  Local<ObjectTemplate> settingt = setting->InstanceTemplate();
  settingt->SetInternalFieldCount(1);
  env->set_http2settings_constructor_template(settingt);

  Local<FunctionTemplate> stream = FunctionTemplate::New(env->isolate());
  stream->SetClassName(FIXED_ONE_BYTE_STRING(env->isolate(), "Http2Stream"));
  env->SetProtoMethod(stream, "id", Http2Stream::GetID);
  env->SetProtoMethod(stream, "destroy", Http2Stream::Destroy);
  env->SetProtoMethod(stream, "flushData", Http2Stream::FlushData);
  env->SetProtoMethod(stream, "priority", Http2Stream::Priority);
  env->SetProtoMethod(stream, "pushPromise", Http2Stream::PushPromise);
  env->SetProtoMethod(stream, "info", Http2Stream::Info);
  env->SetProtoMethod(stream, "respondFD", Http2Stream::RespondFD);
  env->SetProtoMethod(stream, "respond", Http2Stream::Respond);
  env->SetProtoMethod(stream, "rstStream", Http2Stream::RstStream);
  env->SetProtoMethod(stream, "refreshState", Http2Stream::RefreshState);
  AsyncWrap::AddWrapMethods(env, stream);
  StreamBase::AddMethods<Http2Stream>(env, stream, StreamBase::kFlagHasWritev);
  Local<ObjectTemplate> streamt = stream->InstanceTemplate();
  streamt->SetInternalFieldCount(1);
  env->set_http2stream_constructor_template(streamt);
  target->Set(context,
              FIXED_ONE_BYTE_STRING(env->isolate(), "Http2Stream"),
              stream->GetFunction()).FromJust();

  Local<FunctionTemplate> session =
      env->NewFunctionTemplate(Http2Session::New);
  session->SetClassName(http2SessionClassName);
  session->InstanceTemplate()->SetInternalFieldCount(1);
  AsyncWrap::AddWrapMethods(env, session);
  env->SetProtoMethod(session, "ping", Http2Session::Ping);
  env->SetProtoMethod(session, "consume", Http2Session::Consume);
  env->SetProtoMethod(session, "destroy", Http2Session::Destroy);
  env->SetProtoMethod(session, "goaway", Http2Session::Goaway);
  env->SetProtoMethod(session, "settings", Http2Session::Settings);
  env->SetProtoMethod(session, "request", Http2Session::Request);
  env->SetProtoMethod(session, "setNextStreamID",
                      Http2Session::SetNextStreamID);
  env->SetProtoMethod(session, "updateChunksSent",
                      Http2Session::UpdateChunksSent);
  env->SetProtoMethod(session, "refreshState", Http2Session::RefreshState);
  env->SetProtoMethod(
      session, "localSettings",
      Http2Session::RefreshSettings<nghttp2_session_get_local_settings>);
  env->SetProtoMethod(
      session, "remoteSettings",
      Http2Session::RefreshSettings<nghttp2_session_get_remote_settings>);
  target->Set(context,
              http2SessionClassName,
              session->GetFunction()).FromJust();

  Local<Object> constants = Object::New(isolate);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_SESSION_SERVER);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_SESSION_CLIENT);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_STREAM_STATE_IDLE);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_STREAM_STATE_OPEN);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_STREAM_STATE_RESERVED_LOCAL);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_STREAM_STATE_RESERVED_REMOTE);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_STREAM_STATE_HALF_CLOSED_LOCAL);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_STREAM_STATE_HALF_CLOSED_REMOTE);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_STREAM_STATE_CLOSED);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_NO_ERROR);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_PROTOCOL_ERROR);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_INTERNAL_ERROR);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_FLOW_CONTROL_ERROR);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_SETTINGS_TIMEOUT);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_STREAM_CLOSED);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_FRAME_SIZE_ERROR);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_REFUSED_STREAM);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_CANCEL);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_COMPRESSION_ERROR);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_CONNECT_ERROR);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_ENHANCE_YOUR_CALM);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_INADEQUATE_SECURITY);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_HTTP_1_1_REQUIRED);

  NODE_DEFINE_HIDDEN_CONSTANT(constants, NGHTTP2_HCAT_REQUEST);
  NODE_DEFINE_HIDDEN_CONSTANT(constants, NGHTTP2_HCAT_RESPONSE);
  NODE_DEFINE_HIDDEN_CONSTANT(constants, NGHTTP2_HCAT_PUSH_RESPONSE);
  NODE_DEFINE_HIDDEN_CONSTANT(constants, NGHTTP2_HCAT_HEADERS);
  NODE_DEFINE_HIDDEN_CONSTANT(constants, NGHTTP2_NV_FLAG_NONE);
  NODE_DEFINE_HIDDEN_CONSTANT(constants, NGHTTP2_NV_FLAG_NO_INDEX);
  NODE_DEFINE_HIDDEN_CONSTANT(constants, NGHTTP2_ERR_DEFERRED);
  NODE_DEFINE_HIDDEN_CONSTANT(constants, NGHTTP2_ERR_STREAM_ID_NOT_AVAILABLE);
  NODE_DEFINE_HIDDEN_CONSTANT(constants, NGHTTP2_ERR_INVALID_ARGUMENT);
  NODE_DEFINE_HIDDEN_CONSTANT(constants, NGHTTP2_ERR_STREAM_CLOSED);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_ERR_FRAME_SIZE_ERROR);

  NODE_DEFINE_HIDDEN_CONSTANT(constants, STREAM_OPTION_EMPTY_PAYLOAD);
  NODE_DEFINE_HIDDEN_CONSTANT(constants, STREAM_OPTION_GET_TRAILERS);

  NODE_DEFINE_CONSTANT(constants, NGHTTP2_FLAG_NONE);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_FLAG_END_STREAM);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_FLAG_END_HEADERS);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_FLAG_ACK);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_FLAG_PADDED);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_FLAG_PRIORITY);

  NODE_DEFINE_CONSTANT(constants, DEFAULT_SETTINGS_HEADER_TABLE_SIZE);
  NODE_DEFINE_CONSTANT(constants, DEFAULT_SETTINGS_ENABLE_PUSH);
  NODE_DEFINE_CONSTANT(constants, DEFAULT_SETTINGS_INITIAL_WINDOW_SIZE);
  NODE_DEFINE_CONSTANT(constants, DEFAULT_SETTINGS_MAX_FRAME_SIZE);
  NODE_DEFINE_CONSTANT(constants, MAX_MAX_FRAME_SIZE);
  NODE_DEFINE_CONSTANT(constants, MIN_MAX_FRAME_SIZE);
  NODE_DEFINE_CONSTANT(constants, MAX_INITIAL_WINDOW_SIZE);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_DEFAULT_WEIGHT);

  NODE_DEFINE_CONSTANT(constants, NGHTTP2_SETTINGS_HEADER_TABLE_SIZE);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_SETTINGS_ENABLE_PUSH);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_SETTINGS_MAX_FRAME_SIZE);
  NODE_DEFINE_CONSTANT(constants, NGHTTP2_SETTINGS_MAX_HEADER_LIST_SIZE);

  NODE_DEFINE_CONSTANT(constants, PADDING_STRATEGY_NONE);
  NODE_DEFINE_CONSTANT(constants, PADDING_STRATEGY_MAX);
  NODE_DEFINE_CONSTANT(constants, PADDING_STRATEGY_CALLBACK);

#define STRING_CONSTANT(NAME, VALUE)                                          \
  NODE_DEFINE_STRING_CONSTANT(constants, "HTTP2_HEADER_" # NAME, VALUE);
HTTP_KNOWN_HEADERS(STRING_CONSTANT)
#undef STRING_CONSTANT

#define STRING_CONSTANT(NAME, VALUE)                                          \
  NODE_DEFINE_STRING_CONSTANT(constants, "HTTP2_METHOD_" # NAME, VALUE);
HTTP_KNOWN_METHODS(STRING_CONSTANT)
#undef STRING_CONSTANT

#define V(name, _) NODE_DEFINE_CONSTANT(constants, HTTP_STATUS_##name);
HTTP_STATUS_CODES(V)
#undef V

  env->SetMethod(target, "refreshDefaultSettings", RefreshDefaultSettings);
  env->SetMethod(target, "packSettings", PackSettings);

  target->Set(context,
              FIXED_ONE_BYTE_STRING(isolate, "constants"),
              constants).FromJust();
}