TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  if (type == kGenericOptimized) {
    optimized_ops::Floor(GetTensorShape(input), GetTensorData<float>(input),
                         GetTensorShape(output), GetTensorData<float>(output));
  } else {
    reference_ops::Floor(GetTensorShape(input), GetTensorData<float>(input),
                         GetTensorShape(output), GetTensorData<float>(output));
  }

  return kTfLiteOk;
}