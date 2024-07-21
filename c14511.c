TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = static_cast<OpData*>(node->user_data);
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);

  // TODO(b/128934713): Add support for fixed-point per-channel quantization.
  // Currently this only support affine per-layer quantization.
  TF_LITE_ENSURE_EQ(context, output->quantization.type,
                    kTfLiteAffineQuantization);
  const auto* affine_quantization =
      static_cast<TfLiteAffineQuantization*>(output->quantization.params);
  TF_LITE_ENSURE(context, affine_quantization);
  TF_LITE_ENSURE(context, affine_quantization->scale);
  TF_LITE_ENSURE(context, affine_quantization->scale->size == 1);

  if (input->type == kTfLiteFloat32) {
    // Quantize use case.
    TF_LITE_ENSURE(context, output->type == kTfLiteUInt8 ||
                                output->type == kTfLiteInt8 ||
                                output->type == kTfLiteInt16);
  } else {
    // Requantize use case.
    if (input->type == kTfLiteInt16) {
      TF_LITE_ENSURE(
          context, output->type == kTfLiteInt8 || output->type == kTfLiteInt16);
    } else {
      TF_LITE_ENSURE(context,
                     input->type == kTfLiteInt8 || input->type == kTfLiteUInt8);
      TF_LITE_ENSURE(
          context, output->type == kTfLiteUInt8 || output->type == kTfLiteInt8);
    }
    const double effective_output_scale =
        static_cast<double>(input->params.scale) /
        static_cast<double>(output->params.scale);
    QuantizeMultiplier(effective_output_scale, &data->output_multiplier,
                       &data->output_shift);
  }

  return context->ResizeTensor(context, output,
                               TfLiteIntArrayCopy(input->dims));
}