TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, node->inputs->size, 1);
  TF_LITE_ENSURE_EQ(context, node->outputs->size, 1);

  const TfLiteTensor* input_resource_id_tensor =
      GetInput(context, node, kInputVariableId);
  TF_LITE_ENSURE_EQ(context, input_resource_id_tensor->type, kTfLiteInt32);
  TF_LITE_ENSURE_EQ(context, NumElements(input_resource_id_tensor), 1);

  TfLiteTensor* output = GetOutput(context, node, kOutputValue);
  SetTensorToDynamic(output);

  return kTfLiteOk;
}