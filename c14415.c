TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);

  const TfLiteTensor* multipliers = GetInput(context, node, kInputMultipliers);
  // Only int32 and int64 multipliers type is supported.
  if (multipliers->type != kTfLiteInt32 && multipliers->type != kTfLiteInt64) {
    context->ReportError(context,
                         "Multipliers of type '%s' are not supported by tile.",
                         TfLiteTypeGetName(multipliers->type));
    return kTfLiteError;
  }

  if (IsConstantTensor(multipliers)) {
    TF_LITE_ENSURE_OK(context, ResizeOutput(context, node));
  } else {
    SetTensorToDynamic(output);
  }
  return kTfLiteOk;
}