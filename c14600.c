TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  // Check that the inputs and outputs have the right sizes and types.
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 2);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output_values = GetOutput(context, node, kOutputValues);
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output_values->type);

  const TfLiteTensor* top_k = GetInput(context, node, kInputTopK);
  TF_LITE_ENSURE_TYPES_EQ(context, top_k->type, kTfLiteInt32);

  // Set output dynamic if the input is not const.
  if (IsConstantTensor(top_k)) {
    TF_LITE_ENSURE_OK(context, ResizeOutput(context, node));
  } else {
    TfLiteTensor* output_indexes = GetOutput(context, node, kOutputIndexes);
    TfLiteTensor* output_values = GetOutput(context, node, kOutputValues);
    SetTensorToDynamic(output_indexes);
    SetTensorToDynamic(output_values);
  }
  return kTfLiteOk;
}