TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 3);

  OpContext op_context(context, node);

  TF_LITE_ENSURE_EQ(context, NumOutputs(node), op_context.params->num_splits);

  auto input_type = op_context.input->type;
  TF_LITE_ENSURE(context,
                 input_type == kTfLiteFloat32 || input_type == kTfLiteUInt8 ||
                     input_type == kTfLiteInt16 || input_type == kTfLiteInt32 ||
                     input_type == kTfLiteInt64 || input_type == kTfLiteInt8);
  for (int i = 0; i < NumOutputs(node); ++i) {
    GetOutput(context, node, i)->type = input_type;
  }

  auto size_splits = op_context.size_splits;
  TF_LITE_ENSURE_EQ(context, NumDimensions(size_splits), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), NumElements(size_splits));

  // If we know the contents of the 'size_splits' tensor and the 'axis' tensor,
  // resize all outputs. Otherwise, wait until Eval().
  if (IsConstantTensor(op_context.size_splits) &&
      IsConstantTensor(op_context.axis)) {
    return ResizeOutputTensors(context, node, op_context.input,
                               op_context.size_splits, op_context.axis);
  } else {
    return UseDynamicOutputTensors(context, node);
  }
}