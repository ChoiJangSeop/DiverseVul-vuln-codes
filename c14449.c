TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* diag = GetInput(context, node, kDiagonalTensor);
  FillDiagHelper(input, diag, output);
  return kTfLiteOk;
}