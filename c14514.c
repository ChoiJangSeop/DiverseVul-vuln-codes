TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteResizeNearestNeighborParams*>(node->builtin_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  const TfLiteTensor* size = GetInput(context, node, kSizeTensor);

  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_OK(context,
                      ResizeOutputTensor(context, input, size, output));
  }

  tflite::ResizeNearestNeighborParams op_params;
  op_params.align_corners = params->align_corners;
  op_params.half_pixel_centers = params->half_pixel_centers;

  if (output->type == kTfLiteFloat32) {
    reference_ops::ResizeNearestNeighbor(
        op_params, GetTensorShape(input), GetTensorData<int32>(input),
        GetTensorShape(size), GetTensorData<int32>(size),
        GetTensorShape(output), GetTensorData<int32>(output));
  } else if (output->type == kTfLiteUInt8) {
    if (kernel_type == kReference) {
      reference_ops::ResizeNearestNeighbor(
          op_params, GetTensorShape(input), GetTensorData<uint8_t>(input),
          GetTensorShape(size), GetTensorData<int32>(size),
          GetTensorShape(output), GetTensorData<uint8_t>(output));
    }
    if (kernel_type == kGenericOptimized || kernel_type == kNeonOptimized) {
      optimized_ops::ResizeNearestNeighbor(
          op_params, GetTensorShape(input), GetTensorData<uint8_t>(input),
          GetTensorShape(size), GetTensorData<int32>(size),
          GetTensorShape(output), GetTensorData<uint8_t>(output));
    }
  } else if (output->type == kTfLiteInt8) {
    reference_ops::ResizeNearestNeighbor(
        op_params, GetTensorShape(input), GetTensorData<int8_t>(input),
        GetTensorShape(size), GetTensorData<int32>(size),
        GetTensorShape(output), GetTensorData<int8_t>(output));
  } else if (output->type == kTfLiteInt16) {
    reference_ops::ResizeNearestNeighbor(
        op_params, GetTensorShape(input), GetTensorData<int16_t>(input),
        GetTensorShape(size), GetTensorData<int32>(size),
        GetTensorShape(output), GetTensorData<int16_t>(output));
  } else {
    TF_LITE_KERNEL_LOG(
        context, "Output type is %s, requires float, uint8, int8 or int16.",
        TfLiteTypeGetName(output->type));
    return kTfLiteError;
  }

  return kTfLiteOk;
}