TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* cond_tensor =
      GetInput(context, node, kInputConditionTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  if (cond_tensor->type != kTfLiteBool) {
    context->ReportError(context,
                         "Condition tensor must be of type bool, but saw '%s'.",
                         TfLiteTypeGetName(cond_tensor->type));
    return kTfLiteError;
  }

  // As output will be a 2D tensor of indices, use int64 to be consistent with
  // tensorflow.
  output->type = kTfLiteInt64;

  // Exit early if cond is a non-const tensor. Set output tensor to dynamic so
  // output size can be determined in Eval.
  if (!IsConstantTensor(cond_tensor)) {
    SetTensorToDynamic(output);
    return kTfLiteOk;
  }
  return ResizeOutputTensor(context, cond_tensor, output);
}