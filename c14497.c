TfLiteStatus HardSwishEval(TfLiteContext* context, TfLiteNode* node) {
  HardSwishData* data = static_cast<HardSwishData*>(node->user_data);

  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  switch (input->type) {
    case kTfLiteFloat32: {
      if (kernel_type == kReference) {
        reference_ops::HardSwish(
            GetTensorShape(input), GetTensorData<float>(input),
            GetTensorShape(output), GetTensorData<float>(output));
      } else {
        optimized_ops::HardSwish(
            GetTensorShape(input), GetTensorData<float>(input),
            GetTensorShape(output), GetTensorData<float>(output));
      }
      return kTfLiteOk;
    } break;
    case kTfLiteUInt8: {
      HardSwishParams& params = data->params;
      if (kernel_type == kReference) {
        reference_ops::HardSwish(
            params, GetTensorShape(input), GetTensorData<uint8_t>(input),
            GetTensorShape(output), GetTensorData<uint8_t>(output));
      } else {
        optimized_ops::HardSwish(
            params, GetTensorShape(input), GetTensorData<uint8_t>(input),
            GetTensorShape(output), GetTensorData<uint8_t>(output));
      }
      return kTfLiteOk;
    } break;
    case kTfLiteInt8: {
      HardSwishParams& params = data->params;
      if (kernel_type == kReference) {
        reference_ops::HardSwish(
            params, GetTensorShape(input), GetTensorData<int8_t>(input),
            GetTensorShape(output), GetTensorData<int8_t>(output));
      } else {
        optimized_ops::HardSwish(
            params, GetTensorShape(input), GetTensorData<int8_t>(input),
            GetTensorShape(output), GetTensorData<int8_t>(output));
      }
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