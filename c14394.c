TfLiteStatus Resize(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteLSHProjectionParams*>(node->builtin_data);
  TF_LITE_ENSURE(context, NumInputs(node) == 2 || NumInputs(node) == 3);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* hash = GetInput(context, node, 0);
  TF_LITE_ENSURE_EQ(context, NumDimensions(hash), 2);
  // Support up to 32 bits.
  TF_LITE_ENSURE(context, SizeOfDimension(hash, 1) <= 32);

  const TfLiteTensor* input = GetInput(context, node, 1);
  TF_LITE_ENSURE(context, NumDimensions(input) >= 1);

  if (NumInputs(node) == 3) {
    const TfLiteTensor* weight = GetInput(context, node, 2);
    TF_LITE_ENSURE_EQ(context, NumDimensions(weight), 1);
    TF_LITE_ENSURE_EQ(context, SizeOfDimension(weight, 0),
                      SizeOfDimension(input, 0));
  }

  TfLiteTensor* output = GetOutput(context, node, 0);
  TfLiteIntArray* outputSize = TfLiteIntArrayCreate(1);
  switch (params->type) {
    case kTfLiteLshProjectionSparse:
      outputSize->data[0] = SizeOfDimension(hash, 0);
      break;
    case kTfLiteLshProjectionDense:
      outputSize->data[0] = SizeOfDimension(hash, 0) * SizeOfDimension(hash, 1);
      break;
    default:
      return kTfLiteError;
  }
  return context->ResizeTensor(context, output, outputSize);
}