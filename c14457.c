TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  switch (input->type) {
    case kTfLiteInt64:
      reference_ops::Negate(
          GetTensorShape(input), GetTensorData<int64_t>(input),
          GetTensorShape(output), GetTensorData<int64_t>(output));
      break;
    case kTfLiteInt32:
      reference_ops::Negate(
          GetTensorShape(input), GetTensorData<int32_t>(input),
          GetTensorShape(output), GetTensorData<int32_t>(output));
      break;
    case kTfLiteFloat32:
      reference_ops::Negate(GetTensorShape(input), GetTensorData<float>(input),
                            GetTensorShape(output),
                            GetTensorData<float>(output));
      break;
    default:
      context->ReportError(
          context,
          "Neg only currently supports int64, int32, and float32, got %d.",
          input->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}