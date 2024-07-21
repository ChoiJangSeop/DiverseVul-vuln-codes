TfLiteStatus TanhPrepare(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);

  if (kernel_type == kFixedPointOptimized) {
    if (input->type == kTfLiteUInt8 || input->type == kTfLiteInt8) {
      static constexpr int kInputIntegerBits = 4;

      const double input_real_multiplier =
          input->params.scale *
          static_cast<double>(1 << (15 - kInputIntegerBits));

      const double q =
          std::frexp(input_real_multiplier, &data->input_left_shift);
      auto q_fixed = static_cast<int32_t>(TfLiteRound(q * (1ll << 15)));
      data->input_multiplier = static_cast<int16_t>(q_fixed);

      int16_t input_range_radius =
          CalculateInputRadius(kInputIntegerBits, data->input_left_shift, 15);
      data->input_range_radius = input_range_radius;
    }
  }

  if (kernel_type == kGenericOptimized || kernel_type == kReference) {
    if (input->type == kTfLiteUInt8) {
      PopulateLookupTable<uint8_t>(
          data, input, output, [](float value) { return std::tanh(value); });
    } else if (input->type == kTfLiteInt8) {
      PopulateLookupTable<int8_t>(data, input, output,
                                  [](float value) { return std::tanh(value); });
    }
  }

  if (input->type == kTfLiteInt16) {
    static constexpr int kInputIntegerBits = 3;
    static constexpr int kOutputFractionalBits = 15;

    // These operators are implemented in fixed-point arithmetic,
    // which intrinsically wants symmetric ranges (zero_point==0)
    // and power-of-two scales (power-of-two is abbreviated below as POT).
    // While more general support would be possible by means of rescaling,
    // that would add some overhead and some loss of accuracy and wouldn't
    // be used at the moment as current quantized LSTM applications are
    // happy with symmetric, power-of-two-scales quantization. So we just
    // implement that narrow case only for now.

    TF_LITE_ENSURE_EQ(context, input->params.zero_point, 0);
    TF_LITE_ENSURE_EQ(context, output->params.zero_point, 0);

    int input_scale_log2_rounded;
    bool param_scale_pot =
        CheckedLog2(input->params.scale, &input_scale_log2_rounded);

    data->input_left_shift =
        (15 - kInputIntegerBits) + input_scale_log2_rounded;
    param_scale_pot &=
        (data->input_left_shift == 0 || data->input_left_shift == 1);

    if (!param_scale_pot) {
      // In case of general scale parameter, we need to do a rescaling.
      // Magic constant 4096:
      // We need to scale down to (-2^3, 2^3) / 3 is kInputIntegerBits/ interval
      // from 16-bit (-2^15, 2^15),
      // so we need to multiply by
      // 2^(15 - kInputIntegerBits) = 2^12 = 4096.
      data->input_multiplier = static_cast<int32_t>(input->params.scale * 4096);
    }

    int output_scale_log2_rounded;
    TF_LITE_ENSURE(
        context, CheckedLog2(output->params.scale, &output_scale_log2_rounded));
    TF_LITE_ENSURE_EQ(context, output_scale_log2_rounded,
                      -kOutputFractionalBits);
  }

  return context->ResizeTensor(context, output,
                               TfLiteIntArrayCopy(input->dims));
}