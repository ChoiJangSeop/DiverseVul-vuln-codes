TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 3);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 2);

  const TfLiteTensor* lookup = GetInput(context, node, 0);
  TF_LITE_ENSURE_EQ(context, NumDimensions(lookup), 1);
  TF_LITE_ENSURE_EQ(context, lookup->type, kTfLiteInt32);

  const TfLiteTensor* key = GetInput(context, node, 1);
  TF_LITE_ENSURE_EQ(context, NumDimensions(key), 1);
  TF_LITE_ENSURE_EQ(context, key->type, kTfLiteInt32);

  const TfLiteTensor* value = GetInput(context, node, 2);
  TF_LITE_ENSURE(context, NumDimensions(value) >= 1);
  TF_LITE_ENSURE_EQ(context, SizeOfDimension(key, 0),
                    SizeOfDimension(value, 0));
  if (value->type == kTfLiteString) {
    TF_LITE_ENSURE_EQ(context, NumDimensions(value), 1);
  }

  TfLiteTensor* hits = GetOutput(context, node, 1);
  TF_LITE_ENSURE_EQ(context, hits->type, kTfLiteUInt8);
  TfLiteIntArray* hitSize = TfLiteIntArrayCreate(1);
  hitSize->data[0] = SizeOfDimension(lookup, 0);

  TfLiteTensor* output = GetOutput(context, node, 0);
  TF_LITE_ENSURE_EQ(context, value->type, output->type);

  TfLiteStatus status = kTfLiteOk;
  if (output->type != kTfLiteString) {
    TfLiteIntArray* outputSize = TfLiteIntArrayCreate(NumDimensions(value));
    outputSize->data[0] = SizeOfDimension(lookup, 0);
    for (int i = 1; i < NumDimensions(value); i++) {
      outputSize->data[i] = SizeOfDimension(value, i);
    }
    status = context->ResizeTensor(context, output, outputSize);
  }
  if (context->ResizeTensor(context, hits, hitSize) != kTfLiteOk) {
    status = kTfLiteError;
  }
  return status;
}