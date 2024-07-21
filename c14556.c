TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 3);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* begin = GetInput(context, node, kBeginTensor);
  const TfLiteTensor* size = GetInput(context, node, kSizeTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  // Ensure validity of input tensor and its dimension.
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);
  TF_LITE_ENSURE(context,
                 begin->type == kTfLiteInt32 || begin->type == kTfLiteInt64);
  TF_LITE_ENSURE(context,
                 size->type == kTfLiteInt32 || size->type == kTfLiteInt64);
  TF_LITE_ENSURE_EQ(context, NumDimensions(begin), 1);
  TF_LITE_ENSURE_EQ(context, NumDimensions(size), 1);
  TF_LITE_ENSURE_EQ(context, NumElements(begin), NumElements(size));
  TF_LITE_ENSURE_MSG(context, NumDimensions(input) <= kMaxDim,
                     "Slice op only supports 1D-4D input arrays.");

  // Postpone allocation of output if any of the indexing tensors is not
  // constant
  if (!(IsConstantTensor(begin) && IsConstantTensor(size))) {
    SetTensorToDynamic(output);
    return kTfLiteOk;
  }

  return ResizeOutputShape(context, input, begin, size, output);
}