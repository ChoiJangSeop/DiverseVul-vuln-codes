TfLiteStatus ResizeOutput(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  const TfLiteTensor* multipliers = GetInput(context, node, kInputMultipliers);

  const int num_dimensions = NumDimensions(input);
  const int num_multipliers = NumElements(multipliers);
  TF_LITE_ENSURE_EQ(context, num_dimensions, num_multipliers);
  switch (multipliers->type) {
    case kTfLiteInt32:
      return context->ResizeTensor(
          context, output,
          MultiplyShapeDims<int32_t>(*input->dims, multipliers,
                                     num_dimensions));
    case kTfLiteInt64:
      return context->ResizeTensor(
          context, output,
          MultiplyShapeDims<int64_t>(*input->dims, multipliers,
                                     num_dimensions));
    default:
      context->ReportError(
          context, "Multipliers of type '%s' are not supported by tile.",
          TfLiteTypeGetName(multipliers->type));
      return kTfLiteError;
  }
}