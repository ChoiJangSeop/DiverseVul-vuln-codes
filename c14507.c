TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteAddParams*>(node->builtin_data);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input1 = GetInput(context, node, kInputTensor1);
  const TfLiteTensor* input2 = GetInput(context, node, kInputTensor2);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  TF_LITE_ENSURE_TYPES_EQ(context, input1->type, input2->type);
  output->type = input2->type;

  const bool requires_broadcast = !HaveSameShapes(input1, input2);

  TfLiteIntArray* output_size = nullptr;
  if (requires_broadcast) {
    TF_LITE_ENSURE_OK(context, CalculateShapeForBroadcast(
                                   context, input1, input2, &output_size));
  } else {
    output_size = TfLiteIntArrayCopy(input1->dims);
  }

  // 8bit -> 8bit general quantized path, with general rescalings
  // as well as, int16 -> int16 with general rescalings
  bool pot_scale_int16 = true;

  bool input1_scale_is_pot = false;
  bool input2_scale_is_pot = false;
  bool output_scale_is_pot = false;

  int input1_scale_log2_rounded{0};
  int input2_scale_log2_rounded{0};
  int output_scale_log2_rounded{0};

  if (input1->type == kTfLiteInt16 && input2->type == kTfLiteInt16 &&
      output->type == kTfLiteInt16) {
    // In case of 16-bit, there are two implementation:
    // the scale parameter is a general number
    // the scale parameter is POT and
    // zero_point is zero for inputs/output.
    pot_scale_int16 = (input1->params.zero_point == 0) &&
                      (input2->params.zero_point == 0) &&
                      (output->params.zero_point == 0);

    input1_scale_is_pot =
        CheckedLog2(input1->params.scale, &input1_scale_log2_rounded);

    input2_scale_is_pot =
        CheckedLog2(input2->params.scale, &input2_scale_log2_rounded);

    output_scale_is_pot =
        CheckedLog2(output->params.scale, &output_scale_log2_rounded);

    pot_scale_int16 &=
        input1_scale_is_pot && input2_scale_is_pot && output_scale_is_pot;
  }

  data->pot_scale_int16 = pot_scale_int16;

  if (output->type == kTfLiteUInt8 || output->type == kTfLiteInt8 ||
      !pot_scale_int16) {
    // 8bit -> 8bit general quantized path, with general rescalings
    // as well as, 16bit -> 16bit with general rescalings
    data->input1_offset = -input1->params.zero_point;
    data->input2_offset = -input2->params.zero_point;
    data->output_offset = output->params.zero_point;

    // The shift is set to 15 for 16-bit and 20 in case of 8-bit, accordingly.
    // In case of 16-bit we have 65535 << 15 which is less than 1 << 31,
    // therefore the addition will still fit in a 32 bit accumulator.
    data->left_shift = !pot_scale_int16 ? 15 : 20;
    const double twice_max_input_scale =
        2 * std::max(input1->params.scale, input2->params.scale);
    const double real_input1_multiplier =
        input1->params.scale / twice_max_input_scale;
    const double real_input2_multiplier =
        input2->params.scale / twice_max_input_scale;
    const double real_output_multiplier =
        twice_max_input_scale /
        ((1 << data->left_shift) * output->params.scale);

    QuantizeMultiplierSmallerThanOneExp(
        real_input1_multiplier, &data->input1_multiplier, &data->input1_shift);

    QuantizeMultiplierSmallerThanOneExp(
        real_input2_multiplier, &data->input2_multiplier, &data->input2_shift);

    QuantizeMultiplierSmallerThanOneExp(
        real_output_multiplier, &data->output_multiplier, &data->output_shift);

    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
        context, params->activation, output, &data->output_activation_min,
        &data->output_activation_max));
  } else if (output->type == kTfLiteInt16) {
    // 16bit -> 16bit special quantized path, supporting only a rather
    // narrow case of quantization parameters: zero_points must all be 0
    // ("symmetric quantization") and scales must be power-of-two (which
    // we abbreviate as "POT" below). The intended use case for this path
    // is in LSTM cells, where, due to the constraints of implementing
    // some of the math in these LSTM cells in fixed-point arithmetic,
    // we need to have such symmetric, power-of-two quantization
    // (Fixed-point formats are inherently symmetric, power-of-two).
    TF_LITE_ENSURE_EQ(context, input1->params.zero_point, 0);
    TF_LITE_ENSURE_EQ(context, input2->params.zero_point, 0);
    TF_LITE_ENSURE_EQ(context, output->params.zero_point, 0);

    TF_LITE_ENSURE(context, input1_scale_is_pot);
    TF_LITE_ENSURE(context, input2_scale_is_pot);
    TF_LITE_ENSURE(context, output_scale_is_pot);

    data->input1_shift = input1_scale_log2_rounded - output_scale_log2_rounded;
    data->input2_shift = input2_scale_log2_rounded - output_scale_log2_rounded;

    // Shifting of one input is supported. The graph quantization should ensure
    // that the other input matches the output.
    TF_LITE_ENSURE(context, data->input1_shift == 0 || data->input2_shift == 0);
    TF_LITE_ENSURE(context, data->input1_shift <= 0);
    TF_LITE_ENSURE(context, data->input2_shift <= 0);

    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
        context, params->activation, output, &data->output_activation_min,
        &data->output_activation_max));
  }

  return context->ResizeTensor(context, output, output_size);
}