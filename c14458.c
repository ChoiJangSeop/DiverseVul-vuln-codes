TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteLSHProjectionParams*>(node->builtin_data);

  int32_t* out_buf = GetOutput(context, node, 0)->data.i32;
  const TfLiteTensor* hash = GetInput(context, node, 0);
  const TfLiteTensor* input = GetInput(context, node, 1);
  const TfLiteTensor* weight =
      NumInputs(node) == 2 ? nullptr : GetInput(context, node, 2);

  switch (params->type) {
    case kTfLiteLshProjectionDense:
      DenseLshProjection(hash, input, weight, out_buf);
      break;
    case kTfLiteLshProjectionSparse:
      SparseLshProjection(hash, input, weight, out_buf);
      break;
    default:
      return kTfLiteError;
  }

  return kTfLiteOk;
}