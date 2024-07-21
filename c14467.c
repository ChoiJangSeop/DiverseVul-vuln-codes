TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* lookup = GetInput(context, node, 0);
  TF_LITE_ENSURE_EQ(context, NumDimensions(lookup), 1);
  TF_LITE_ENSURE_EQ(context, lookup->type, kTfLiteInt32);

  const TfLiteTensor* value = GetInput(context, node, 1);
  TF_LITE_ENSURE(context, NumDimensions(value) >= 2);

  TfLiteTensor* output = GetOutput(context, node, 0);
  TfLiteIntArray* outputSize = TfLiteIntArrayCreate(NumDimensions(value));

  outputSize->data[0] = SizeOfDimension(lookup, 0);
  outputSize->data[1] = SizeOfDimension(value, 1);
  for (int i = 2; i < NumDimensions(value); i++) {
    outputSize->data[i] = SizeOfDimension(value, i);
  }
  return context->ResizeTensor(context, output, outputSize);
}