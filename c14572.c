TfLiteStatus Relu1Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  const ReluOpData* data = reinterpret_cast<ReluOpData*>(node->user_data);
  switch (input->type) {
    case kTfLiteFloat32: {
      optimized_ops::Relu1(GetTensorShape(input), GetTensorData<float>(input),
                           GetTensorShape(output),
                           GetTensorData<float>(output));
      return kTfLiteOk;
    } break;
    case kTfLiteUInt8: {
      QuantizedReluX<uint8_t>(-1.0f, 1.0f, input, output, data);
      return kTfLiteOk;
    } break;
    case kTfLiteInt8: {
      QuantizedReluX<int8_t>(-1, 1, input, output, data);
      return kTfLiteOk;
    } break;
    default:
      TF_LITE_KERNEL_LOG(context,
                         "Only float32, uint8, int8 supported "
                         "currently, got %s.",
                         TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
}