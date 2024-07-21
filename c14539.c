TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 4);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* indices = GetInput(context, node, kIndicesTensor);
  const TfLiteTensor* output_shape =
      GetInput(context, node, kOutputShapeTensor);
  const TfLiteTensor* values = GetInput(context, node, kValueInputTensor);
  const TfLiteTensor* default_value =
      GetInput(context, node, kDefaultValueTensor);

  // TODO(renjieliu): Handle validate_indices.

  // Indices can be 0-D, 1-D or 2-D.
  TF_LITE_ASSERT(NumDimensions(indices) >= 0);
  TF_LITE_ENSURE(context, NumDimensions(indices) < 3);
  TF_LITE_ASSERT(NumDimensions(output_shape) >= 0);
  TF_LITE_ENSURE_EQ(context, NumDimensions(output_shape), 1);
  // Values can be 0-D or 1-D.
  TF_LITE_ASSERT(NumDimensions(values) >= 0);
  TF_LITE_ENSURE(context, NumDimensions(values) < 2);

  TF_LITE_ENSURE_EQ(context, NumElements(default_value), 1);

  TF_LITE_ENSURE(
      context, indices->type == kTfLiteInt32 || indices->type == kTfLiteInt64);
  TF_LITE_ENSURE(context, output_shape->type == kTfLiteInt32 ||
                              output_shape->type == kTfLiteInt64);
  TF_LITE_ENSURE(context, values->type == kTfLiteInt32 ||
                              values->type == kTfLiteInt64 ||
                              values->type == kTfLiteInt8 ||
                              values->type == kTfLiteUInt8 ||
                              values->type == kTfLiteFloat32);
  TF_LITE_ENSURE_TYPES_EQ(context, values->type, default_value->type);

  // Ensure dimensions match.
  TF_LITE_ENSURE_OK(
      context, CheckDimensionsMatch(context, indices, output_shape, values));

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  output->type = values->type;
  TF_LITE_ENSURE_EQ(context, NumDimensions(output_shape), 1);

  if (!IsConstantTensor(output_shape)) {
    SetTensorToDynamic(output);
    return kTfLiteOk;
  }
  return ResizeOutputShape(context, output_shape, output);
}