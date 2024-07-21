TfLiteStatus HardSwishPrepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_STATUS(GenericPrepare(context, node));
  TfLiteTensor* output = GetOutput(context, node, 0);

  if (output->type == kTfLiteUInt8 || output->type == kTfLiteInt8) {
    HardSwishData* data = static_cast<HardSwishData*>(node->user_data);
    HardSwishParams* params = &data->params;
    const TfLiteTensor* input = GetInput(context, node, 0);
    params->input_zero_point = input->params.zero_point;
    params->output_zero_point = output->params.zero_point;
    const float input_scale = input->params.scale;
    const float hires_input_scale = (1.0f / 128.0f) * input_scale;
    const float reluish_scale = 3.0f / 32768.0f;
    const float output_scale = output->params.scale;

    const float output_multiplier = hires_input_scale / output_scale;

    int32_t output_multiplier_fixedpoint_int32;
    QuantizeMultiplier(output_multiplier, &output_multiplier_fixedpoint_int32,
                       &params->output_multiplier_exponent);
    DownScaleInt32ToInt16Multiplier(
        output_multiplier_fixedpoint_int32,
        &params->output_multiplier_fixedpoint_int16);
    TF_LITE_ENSURE(context, params->output_multiplier_exponent <= 0);

    const float reluish_multiplier = hires_input_scale / reluish_scale;
    int32_t reluish_multiplier_fixedpoint_int32;
    QuantizeMultiplier(reluish_multiplier, &reluish_multiplier_fixedpoint_int32,
                       &params->reluish_multiplier_exponent);
    DownScaleInt32ToInt16Multiplier(
        reluish_multiplier_fixedpoint_int32,
        &params->reluish_multiplier_fixedpoint_int16);
  }
  return kTfLiteOk;
}