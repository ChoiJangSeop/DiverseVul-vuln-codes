TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* size = GetInput(context, node, kSizeTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  // TODO(ahentz): Our current implementations rely on the input being 4D,
  // and the size being 1D tensor with exactly 2 elements.
  TF_LITE_ENSURE_EQ(context, NumDimensions(input), 4);
  TF_LITE_ENSURE_EQ(context, NumDimensions(size), 1);
  TF_LITE_ENSURE_TYPES_EQ(context, size->type, kTfLiteInt32);
  TF_LITE_ENSURE_EQ(context, size->dims->data[0], 2);

  output->type = input->type;

  if (!IsConstantTensor(size)) {
    SetTensorToDynamic(output);
    return kTfLiteOk;
  }
  return ResizeOutputTensor(context, input, size, output);
}