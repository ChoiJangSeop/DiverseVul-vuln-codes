TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  TF_LITE_ENSURE_TYPES_EQ(context, GetInput(context, node, 0)->type,
                          kTfLiteString);
  TF_LITE_ENSURE_TYPES_EQ(context, GetOutput(context, node, 0)->type,
                          kTfLiteString);
  return kTfLiteOk;
}