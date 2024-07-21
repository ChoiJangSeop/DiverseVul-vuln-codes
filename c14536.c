TfLiteStatus PrepareAny(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  const TfLiteTensor* input = GetInput(context, node, 0);
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, kTfLiteBool);
  return PrepareSimple(context, node);
}