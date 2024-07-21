TfLiteTensor* GetTempLhs(TfLiteContext* context, TfLiteNode* node,
                         const TfLiteTensor* lhs) {
  TfLiteTensor* transposed_lhs = GetTemporary(context, node, 0);
  if (lhs->type == kTfLiteInt8) {
    // Get the quantization params from the LHS tensor.
    transposed_lhs->params.scale = lhs->params.scale;
    transposed_lhs->params.zero_point = lhs->params.zero_point;
  }
  return transposed_lhs;
}