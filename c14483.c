TfLiteStatus SoftmaxPrepare(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteSoftmaxParams*>(node->builtin_data);
  SoftmaxOpData* data = reinterpret_cast<SoftmaxOpData*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  if (output->type == kTfLiteInt16) {
    TF_LITE_ENSURE(context, input->type == kTfLiteInt8 ||
                                input->type == kTfLiteUInt8 ||
                                input->type == kTfLiteInt16);
  } else {
    TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);
  }

  TF_LITE_ENSURE(context, NumDimensions(input) >= 1);

  if (input->type == kTfLiteUInt8 || input->type == kTfLiteInt8) {
    switch (output->type) {
      case kTfLiteUInt8:
      case kTfLiteInt8:
#ifdef TFLITE_SOFTMAX_USE_UINT16_LUT
        // Only apply when both input & output are uint8/int8 & build with clang
        // on aarch64.
        // TODO(b/143709993): Port to ARMv7 and other platforms.
        data->params.uint8_table1 = data->uint8_table1;
        data->params.uint8_table2 = data->uint8_table2;
        optimized_ops::PopulateSoftmaxUInt8LookupTable(
            &data->params, input->params.scale, params->beta);
        break;
#endif
      case kTfLiteInt16:
      default:
        data->params.table = data->table;
        optimized_ops::PopulateSoftmaxLookupTable(
            &data->params, input->params.scale, params->beta);
    }

    data->params.zero_point = output->params.zero_point;
    data->params.scale = output->params.scale;
  }

  if (input->type == kTfLiteInt16) {
    TF_LITE_ENSURE_EQ(context, output->params.zero_point, 0);

    data->params.exp_lut = data->exp_lut;
    // exp LUT only used on nagative values
    // we consider exp(-10.0) is insignificant to accumulation
    gen_lut([](double value) { return std::exp(value); }, -10.0, 0.0,
            data->params.exp_lut, data->kInt16LUTArraySize);
    data->params.one_over_one_plus_x_lut = data->one_over_one_plus_x_lut;
    gen_lut([](double value) { return 1.0 / (1.0 + value); }, 0.0, 1.0,
            data->params.one_over_one_plus_x_lut, data->kInt16LUTArraySize);
    data->params.zero_point = output->params.zero_point;
    data->params.scale = output->params.scale;

    double input_scale_beta_rescale =
        input->params.scale * params->beta /
        (10.0 / 65535.0);  // scale the input_diff such that [-65535, 0]
                           // correspond to [-10.0, 0.0]
    QuantizeMultiplier(input_scale_beta_rescale, &data->params.input_multiplier,
                       &data->params.input_left_shift);
  }

  return context->ResizeTensor(context, output,
                               TfLiteIntArrayCopy(input->dims));
}