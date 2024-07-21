TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* input = GetInput(context, node, kInput);
  const TfLiteTensor* axis = GetInput(context, node, kAxis);
  TfLiteTensor* output = GetOutput(context, node, 0);
  output->type = input->type;
  if (IsConstantTensor(axis)) {
    int axis_value;
    TF_LITE_ENSURE_OK(context,
                      GetAxisValueFromTensor(context, *axis, &axis_value));
    return ExpandTensorDim(context, *input, axis_value, output);
  }
  SetTensorToDynamic(output);
  return kTfLiteOk;
}