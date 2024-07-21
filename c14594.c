TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE(context, NumInputs(node) == 1 || NumInputs(node) == 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  // Always postpone sizing string tensors, even if we could in principle
  // calculate their shapes now. String tensors don't benefit from having their
  // shapes precalculated because the actual memory can only be allocated after
  // we know all the content.
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  if (output->type != kTfLiteString) {
    if (NumInputs(node) == 1 ||
        IsConstantTensor(GetInput(context, node, kShapeTensor))) {
      TF_LITE_ENSURE_OK(context, ResizeOutput(context, node));
    } else {
      SetTensorToDynamic(output);
    }
  }
  return kTfLiteOk;
}