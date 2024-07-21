TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  if (input->type != kTfLiteFloat32) {
    TF_LITE_UNSUPPORTED_TYPE(context, input->type, "Ceil");
  }

  optimized_ops::Ceil(GetTensorShape(input), GetTensorData<float>(input),
                      GetTensorShape(output), GetTensorData<float>(output));

  return kTfLiteOk;
}