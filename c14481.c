TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  optimized_ops::Round(GetTensorShape(input), GetTensorData<float>(input),
                       GetTensorShape(output), GetTensorData<float>(output));

  return kTfLiteOk;
}