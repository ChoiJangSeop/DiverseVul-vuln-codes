TfLiteStatus PrepareMeanOrSum(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_OK(context, PrepareSimple(context, node));
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  // reduce_mean requires a buffer to store intermediate sum result.
  OpContext op_context(context, node);
  if (op_context.input->type == kTfLiteInt8 ||
      op_context.input->type == kTfLiteUInt8 ||
      op_context.input->type == kTfLiteInt16) {
    const double real_multiplier =
        static_cast<double>(op_context.input->params.scale) /
        static_cast<double>(op_context.output->params.scale);
    int exponent;
    QuantizeMultiplier(real_multiplier, &data->multiplier, &exponent);
    data->shift = exponent;
  }
  TfLiteTensor* temp_sum = GetTemporary(context, node, /*index=*/2);
  if (!IsConstantTensor(op_context.axis)) {
    SetTensorToDynamic(temp_sum);
    return kTfLiteOk;
  }
  temp_sum->allocation_type = kTfLiteArenaRw;
  return ResizeTempSum(context, &op_context, temp_sum);
}