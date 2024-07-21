TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  // TODO(b/137042749): TFLite infrastructure (converter, delegate) doesn't
  // fully support 0-output ops yet. Currently it works if we manually crfat
  // a TFLite graph that contains variable ops. Note:
  // * The TFLite Converter need to be changed to be able to produce an op
  //   with 0 output.
  // * The delegation code need to be changed to handle 0 output ops. However
  //   everything still works fine when variable ops aren't used.
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 0);

  const TfLiteTensor* input_resource_id_tensor =
      GetInput(context, node, kInputVariableId);
  TF_LITE_ENSURE_EQ(context, input_resource_id_tensor->type, kTfLiteInt32);
  TF_LITE_ENSURE_EQ(context, NumElements(input_resource_id_tensor), 1);

  return kTfLiteOk;
}