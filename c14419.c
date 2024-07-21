TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  const int num_elements = NumElements(input);
  TF_LITE_ENSURE_EQ(context, num_elements, NumElements(output));
  switch (input->type) {
    case kTfLiteInt64:
      return copyToTensor(context, input->data.i64, output, num_elements);
    case kTfLiteInt32:
      return copyToTensor(context, input->data.i32, output, num_elements);
    case kTfLiteUInt8:
      return copyToTensor(context, input->data.uint8, output, num_elements);
    case kTfLiteFloat32:
      return copyToTensor(context, GetTensorData<float>(input), output,
                          num_elements);
    case kTfLiteBool:
      return copyToTensor(context, input->data.b, output, num_elements);
    case kTfLiteComplex64:
      return copyToTensor(
          context, reinterpret_cast<std::complex<float>*>(input->data.c64),
          output, num_elements);
    default:
      // Unsupported type.
      TF_LITE_UNSUPPORTED_TYPE(context, input->type, "Cast");
  }
  return kTfLiteOk;
}