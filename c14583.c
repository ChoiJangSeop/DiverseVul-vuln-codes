TfLiteStatus EvalImpl(TfLiteContext* context, const TfLiteTensor* input,
                      TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteUniqueParams*>(node->builtin_data);
  if (params == nullptr) {
    context->ReportError(context, "Null params passed");
    return kTfLiteError;
  }
  switch (params->index_out_type) {
    case kTfLiteInt32:
      return EvalImpl<T, int32_t>(context, input, node);
    case kTfLiteInt64:
      return EvalImpl<T, int64_t>(context, input, node);
    default:
      context->ReportError(
          context,
          "Unique index output array can only be Int32 or In64, requested: %s",
          TfLiteTypeGetName(params->index_out_type));
  }
  return kTfLiteError;
}