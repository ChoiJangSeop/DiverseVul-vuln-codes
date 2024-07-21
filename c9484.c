absl::optional<CelValue> evaluate(const Expression& expr, Protobuf::Arena* arena,
                                  const StreamInfo::StreamInfo& info,
                                  const Http::RequestHeaderMap* request_headers,
                                  const Http::ResponseHeaderMap* response_headers,
                                  const Http::ResponseTrailerMap* response_trailers) {
  auto activation = createActivation(info, request_headers, response_headers, response_trailers);
  auto eval_status = expr.Evaluate(*activation, arena);
  if (!eval_status.ok()) {
    return {};
  }

  return eval_status.value();
}