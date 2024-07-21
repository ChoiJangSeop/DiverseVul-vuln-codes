TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteSpaceToDepthParams*>(node->builtin_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

#define TF_LITE_SPACE_TO_DEPTH(type, scalar)                               \
  tflite::SpaceToDepthParams op_params;                                    \
  op_params.block_size = params->block_size;                               \
  type::SpaceToDepth(op_params, GetTensorShape(input),                     \
                     GetTensorData<scalar>(input), GetTensorShape(output), \
                     GetTensorData<scalar>(output))
  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32:
      if (kernel_type == kReference) {
        TF_LITE_SPACE_TO_DEPTH(reference_ops, float);
      } else {
        TF_LITE_SPACE_TO_DEPTH(optimized_ops, float);
      }
      break;
    case kTfLiteUInt8:
      if (kernel_type == kReference) {
        TF_LITE_SPACE_TO_DEPTH(reference_ops, uint8_t);
      } else {
        TF_LITE_SPACE_TO_DEPTH(optimized_ops, uint8_t);
      }
      break;
    case kTfLiteInt8:
      if (kernel_type == kReference) {
        TF_LITE_SPACE_TO_DEPTH(reference_ops, int8_t);
      } else {
        TF_LITE_SPACE_TO_DEPTH(optimized_ops, int8_t);
      }
      break;
    case kTfLiteInt32:
      if (kernel_type == kReference) {
        TF_LITE_SPACE_TO_DEPTH(reference_ops, int32_t);
      } else {
        TF_LITE_SPACE_TO_DEPTH(optimized_ops, int32_t);
      }
      break;
    case kTfLiteInt64:
      if (kernel_type == kReference) {
        TF_LITE_SPACE_TO_DEPTH(reference_ops, int64_t);
      } else {
        TF_LITE_SPACE_TO_DEPTH(optimized_ops, int64_t);
      }
      break;
    default:
      context->ReportError(context, "Type '%s' not currently supported.",
                           TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
#undef TF_LITE_SPACE_TO_DEPTH

  return kTfLiteOk;
}