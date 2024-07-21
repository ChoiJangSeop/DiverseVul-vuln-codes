TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* params = GetInput(context, node, kParams);
  const TfLiteTensor* indices = GetInput(context, node, kIndices);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (indices->type) {
    case kTfLiteInt32:
      return EvalGatherNd<int32_t>(context, params, indices, output);
    case kTfLiteInt64:
      return EvalGatherNd<int64_t>(context, params, indices, output);
    default:
      context->ReportError(
          context, "Indices of type '%s' are not supported by gather_nd.",
          TfLiteTypeGetName(indices->type));
      return kTfLiteError;
  }
}