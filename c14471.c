TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* axis = GetInput(context, node, kAxisTensor);
  TF_LITE_ENSURE_EQ(context, NumDimensions(axis), 1);
  TF_LITE_ENSURE(context, NumDimensions(input) >= NumElements(axis));

  if (input->type != kTfLiteInt32 && input->type != kTfLiteFloat32 &&
      input->type != kTfLiteUInt8 && input->type != kTfLiteInt16 &&
      input->type != kTfLiteInt64 && input->type != kTfLiteBool) {
    context->ReportError(context, "Type '%s' is not supported by reverse.",
                         TfLiteTypeGetName(input->type));
    return kTfLiteError;
  }

  if (axis->type != kTfLiteInt32) {
    context->ReportError(context, "Axis Type '%s' is not supported by reverse.",
                         TfLiteTypeGetName(axis->type));
    return kTfLiteError;
  }

  // TODO(renjieliu): support multi-axis case.
  if (NumElements(axis) > 1) {
    context->ReportError(context, "Current does not support more than 1 axis.");
  }

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TfLiteIntArray* output_shape = TfLiteIntArrayCopy(input->dims);
  TF_LITE_ENSURE_TYPES_EQ(context, output->type, input->type);

  return context->ResizeTensor(context, output, output_shape);
}