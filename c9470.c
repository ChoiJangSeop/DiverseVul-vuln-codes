bool matches(const Expression& expr, const StreamInfo::StreamInfo& info,
             const Http::RequestHeaderMap& headers) {
  Protobuf::Arena arena;
  auto eval_status = Expr::evaluate(expr, &arena, info, &headers, nullptr, nullptr);
  if (!eval_status.has_value()) {
    return false;
  }
  auto result = eval_status.value();
  return result.IsBool() ? result.BoolOrDie() : false;
}