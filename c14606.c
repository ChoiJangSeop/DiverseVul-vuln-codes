TfLiteStatus Relu6Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  ReluOpData* data = reinterpret_cast<ReluOpData*>(node->user_data);
  switch (input->type) {
    case kTfLiteFloat32: {
      size_t elements = input->bytes / sizeof(float);
      const float* in = GetTensorData<float>(input);
      const float* in_end = in + elements;
      float* out = GetTensorData<float>(output);
      for (; in < in_end; in++, out++) *out = std::min(std::max(0.f, *in), 6.f);
      return kTfLiteOk;
    } break;
    case kTfLiteUInt8:
      QuantizedReluX<uint8_t>(0.0f, 6.0f, input, output, data);
      return kTfLiteOk;
    case kTfLiteInt8: {
      QuantizedReluX<int8_t>(0.0f, 6.0f, input, output, data);
      return kTfLiteOk;
    } break;
    default:
      TF_LITE_KERNEL_LOG(
          context,
          "Only float32, uint8 and int8 are supported currently, got %s.",
          TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
}