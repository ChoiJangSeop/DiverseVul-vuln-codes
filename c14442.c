TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  // Just copy input to output.
  const TfLiteTensor* input = GetInput(context, node, kInput);
  TfLiteTensor* output = GetOutput(context, node, 0);
  const TfLiteTensor* axis = GetInput(context, node, kAxis);
  if (IsDynamicTensor(output)) {
    int axis_value;
    TF_LITE_ENSURE_OK(context,
                      GetAxisValueFromTensor(context, *axis, &axis_value));
    TF_LITE_ENSURE_OK(context,
                      ExpandTensorDim(context, *input, axis_value, output));
  }
  if (output->type == kTfLiteString) {
    TfLiteTensorRealloc(input->bytes, output);
  }
  memcpy(output->data.raw, input->data.raw, input->bytes);
  return kTfLiteOk;
}