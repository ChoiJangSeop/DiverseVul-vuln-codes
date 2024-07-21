TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);

  OpContext op_context(context, node);

  TF_LITE_ENSURE_EQ(context, NumOutputs(node), op_context.params->num_splits);

  auto input_type = op_context.input->type;
  TF_LITE_ENSURE(context,
                 input_type == kTfLiteFloat32 || input_type == kTfLiteUInt8 ||
                     input_type == kTfLiteInt8 || input_type == kTfLiteInt16 ||
                     input_type == kTfLiteInt32);
  for (int i = 0; i < NumOutputs(node); ++i) {
    GetOutput(context, node, i)->type = input_type;
  }

  // If we know the contents of the 'axis' tensor, resize all outputs.
  // Otherwise, wait until Eval().
  if (IsConstantTensor(op_context.axis)) {
    return ResizeOutputTensors(context, node, op_context.axis, op_context.input,
                               op_context.params->num_splits);
  } else {
    return UseDynamicOutputTensors(context, node);
  }
}