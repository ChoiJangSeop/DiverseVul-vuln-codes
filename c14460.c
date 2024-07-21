TfLiteStatus ReluPrepare(TfLiteContext* context, TfLiteNode* node) {
  ReluOpData* data = reinterpret_cast<ReluOpData*>(node->user_data);
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);

  if (input->type == kTfLiteInt8 || input->type == kTfLiteUInt8) {
    double real_multiplier = input->params.scale / output->params.scale;
    QuantizeMultiplier(real_multiplier, &data->output_multiplier,
                       &data->output_shift);
  }

  return context->ResizeTensor(context, output,
                               TfLiteIntArrayCopy(input->dims));
}