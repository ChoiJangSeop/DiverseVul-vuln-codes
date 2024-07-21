TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 5);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* ids = GetInput(context, node, 0);
  TF_LITE_ENSURE_EQ(context, NumDimensions(ids), 1);
  TF_LITE_ENSURE_EQ(context, ids->type, kTfLiteInt32);

  const TfLiteTensor* indices = GetInput(context, node, 1);
  TF_LITE_ENSURE_EQ(context, NumDimensions(indices), 2);
  TF_LITE_ENSURE_EQ(context, indices->type, kTfLiteInt32);

  const TfLiteTensor* shape = GetInput(context, node, 2);
  TF_LITE_ENSURE_EQ(context, NumDimensions(shape), 1);
  TF_LITE_ENSURE_EQ(context, shape->type, kTfLiteInt32);

  const TfLiteTensor* weights = GetInput(context, node, 3);
  TF_LITE_ENSURE_EQ(context, NumDimensions(weights), 1);
  TF_LITE_ENSURE_EQ(context, weights->type, kTfLiteFloat32);

  TF_LITE_ENSURE_EQ(context, SizeOfDimension(indices, 0),
                    SizeOfDimension(ids, 0));
  TF_LITE_ENSURE_EQ(context, SizeOfDimension(indices, 0),
                    SizeOfDimension(weights, 0));

  const TfLiteTensor* value = GetInput(context, node, 4);
  TF_LITE_ENSURE(context, NumDimensions(value) >= 2);

  // Mark the output as a dynamic tensor.
  TfLiteTensor* output = GetOutput(context, node, 0);
  TF_LITE_ENSURE_TYPES_EQ(context, output->type, kTfLiteFloat32);
  output->allocation_type = kTfLiteDynamic;

  return kTfLiteOk;
}