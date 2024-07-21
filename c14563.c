TfLiteStatus LeakyReluEval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  const auto* params =
      reinterpret_cast<TfLiteLeakyReluParams*>(node->builtin_data);
  const LeakyReluOpData* data =
      reinterpret_cast<LeakyReluOpData*>(node->user_data);

  LeakyReluParams op_params;
  switch (input->type) {
    case kTfLiteFloat32: {
      op_params.alpha = params->alpha;
      optimized_ops::LeakyRelu(
          op_params, GetTensorShape(input), GetTensorData<float>(input),
          GetTensorShape(output), GetTensorData<float>(output));
      return kTfLiteOk;
    } break;
    case kTfLiteUInt8: {
      QuantizeLeakyRelu<uint8_t>(input, output, data);
      return kTfLiteOk;
    } break;
    case kTfLiteInt8: {
      QuantizeLeakyRelu<int8_t>(input, output, data);
      return kTfLiteOk;
    } break;
    case kTfLiteInt16: {
      QuantizeLeakyRelu<int16_t>(input, output, data);
      return kTfLiteOk;
    } break;
    default:
      TF_LITE_KERNEL_LOG(
          context,
          "Only float32, int8, int16 and uint8 is supported currently, got %s.",
          TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
}