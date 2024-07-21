TfLiteTensor* GetTempRhs(TfLiteContext* context, TfLiteNode* node,
                         const TfLiteTensor* rhs) {
  TfLiteTensor* transposed_rhs = GetTemporary(context, node, 1);
  if (rhs->type == kTfLiteInt8) {
    // Get the quantization params from the RHS tensor.
    transposed_rhs->params.scale = rhs->params.scale;
    transposed_rhs->params.zero_point = rhs->params.zero_point;
  }
  return transposed_rhs;
}