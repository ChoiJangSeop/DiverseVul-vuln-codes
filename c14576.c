TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* size = GetInput(context, node, kSizeTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  // TODO(ahentz): Our current implementations rely on the inputs being 4D.
  TF_LITE_ENSURE_EQ(context, NumDimensions(input), 4);
  TF_LITE_ENSURE_EQ(context, NumDimensions(size), 1);

  TF_LITE_ENSURE_EQ(context, size->type, kTfLiteInt32);
  // ResizeBilinear creates a float tensor even when the input is made of
  // integers.
  output->type = input->type;

  if (!IsConstantTensor(size)) {
    SetTensorToDynamic(output);
    return kTfLiteOk;
  }

  // Ensure params are valid.
  auto* params =
      reinterpret_cast<TfLiteResizeBilinearParams*>(node->builtin_data);
  if (params->half_pixel_centers && params->align_corners) {
    context->ReportError(
        context, "If half_pixel_centers is True, align_corners must be False.");
    return kTfLiteError;
  }

  return ResizeOutputTensor(context, input, size, output);
}