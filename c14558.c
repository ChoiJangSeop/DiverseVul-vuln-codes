TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  const int num_elements = NumElements(input);
  switch (input->type) {
    case kTfLiteInt64:
      memset(GetTensorData<int64_t>(output), 0, num_elements * sizeof(int64_t));
      break;
    case kTfLiteInt32:
      memset(GetTensorData<int32_t>(output), 0, num_elements * sizeof(int32_t));
      break;
    case kTfLiteFloat32:
      memset(GetTensorData<float>(output), 0, num_elements * sizeof(float));
      break;
    default:
      context->ReportError(context,
                           "ZerosLike only currently supports int64, int32, "
                           "and float32, got %d.",
                           input->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}