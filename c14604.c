TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteResizeBilinearParams*>(node->builtin_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  const TfLiteTensor* size = GetInput(context, node, kSizeTensor);

  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_OK(context,
                      ResizeOutputTensor(context, input, size, output));
  }

  if (output->type == kTfLiteFloat32) {
#define TF_LITE_RESIZE_BILINEAR(type, datatype)                              \
  tflite::ResizeBilinearParams op_params;                                    \
  op_params.align_corners = params->align_corners;                           \
  op_params.half_pixel_centers = params->half_pixel_centers;                 \
  type::ResizeBilinear(op_params, GetTensorShape(input),                     \
                       GetTensorData<datatype>(input), GetTensorShape(size), \
                       GetTensorData<int32>(size), GetTensorShape(output),   \
                       GetTensorData<datatype>(output))

    if (kernel_type == kReference) {
      TF_LITE_RESIZE_BILINEAR(reference_ops, float);
    }
    if (kernel_type == kGenericOptimized || kernel_type == kNeonOptimized) {
      TF_LITE_RESIZE_BILINEAR(optimized_ops, float);
    }
  } else if (output->type == kTfLiteUInt8) {
    if (kernel_type == kReference) {
      TF_LITE_RESIZE_BILINEAR(reference_ops, uint8_t);
    }
    if (kernel_type == kGenericOptimized || kernel_type == kNeonOptimized) {
      TF_LITE_RESIZE_BILINEAR(optimized_ops, uint8_t);
    }
  } else if (output->type == kTfLiteInt8) {
    TF_LITE_RESIZE_BILINEAR(reference_ops, int8_t);
#undef TF_LITE_RESIZE_BILINEAR
  } else {
    context->ReportError(context, "Output type is %d, requires float.",
                         output->type);
    return kTfLiteError;
  }

  return kTfLiteOk;
}