TfLiteStatus ReluEval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  const ReluOpData* data = reinterpret_cast<ReluOpData*>(node->user_data);
  switch (input->type) {
    case kTfLiteFloat32: {
      optimized_ops::Relu(GetTensorShape(input), GetTensorData<float>(input),
                          GetTensorShape(output), GetTensorData<float>(output));
    } break;
    // TODO(renjieliu): We may revisit the quantization calculation logic,
    // the unbounded upper limit is actually hard to quantize.
    case kTfLiteUInt8: {
      QuantizedReluX<uint8_t>(0.0f, std::numeric_limits<float>::infinity(),
                              input, output, data);
    } break;
    case kTfLiteInt8: {
      QuantizedReluX<int8_t>(0.0f, std::numeric_limits<float>::infinity(),
                             input, output, data);
    } break;
    default:
      TF_LITE_KERNEL_LOG(
          context, "Only float32 & int8/uint8 is supported currently, got %s.",
          TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}