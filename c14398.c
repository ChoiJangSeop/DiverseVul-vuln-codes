TfLiteStatus PopulateQuantizedLstmParams8x8_8(
    TfLiteContext* context, TfLiteNode* node,
    lstm_eval::IntegerLstmParameter* integer_lstm_param) {
  // Get all tensors.
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* input_to_input_weights =
      GetOptionalInputTensor(context, node, kInputToInputWeightsTensor);
  const TfLiteTensor* input_to_forget_weights =
      GetInput(context, node, kInputToForgetWeightsTensor);
  const TfLiteTensor* input_to_cell_weights =
      GetInput(context, node, kInputToCellWeightsTensor);
  const TfLiteTensor* input_to_output_weights =
      GetInput(context, node, kInputToOutputWeightsTensor);

  const TfLiteTensor* recurrent_to_input_weights =
      GetOptionalInputTensor(context, node, kRecurrentToInputWeightsTensor);
  const TfLiteTensor* recurrent_to_forget_weights =
      GetInput(context, node, kRecurrentToForgetWeightsTensor);
  const TfLiteTensor* recurrent_to_cell_weights =
      GetInput(context, node, kRecurrentToCellWeightsTensor);
  const TfLiteTensor* recurrent_to_output_weights =
      GetInput(context, node, kRecurrentToOutputWeightsTensor);

  const TfLiteTensor* cell_to_input_weights =
      GetOptionalInputTensor(context, node, kCellToInputWeightsTensor);
  const TfLiteTensor* cell_to_forget_weights =
      GetOptionalInputTensor(context, node, kCellToForgetWeightsTensor);
  const TfLiteTensor* cell_to_output_weights =
      GetOptionalInputTensor(context, node, kCellToOutputWeightsTensor);

  const TfLiteTensor* input_layer_norm_coefficients =
      GetOptionalInputTensor(context, node, kInputLayerNormCoefficientsTensor);
  const TfLiteTensor* forget_layer_norm_coefficients =
      GetOptionalInputTensor(context, node, kForgetLayerNormCoefficientsTensor);
  const TfLiteTensor* cell_layer_norm_coefficients =
      GetOptionalInputTensor(context, node, kCellLayerNormCoefficientsTensor);
  const TfLiteTensor* output_layer_norm_coefficients =
      GetOptionalInputTensor(context, node, kOutputLayerNormCoefficientsTensor);

  const TfLiteTensor* input_gate_bias =
      GetOptionalInputTensor(context, node, kInputGateBiasTensor);
  const TfLiteTensor* forget_gate_bias =
      GetInput(context, node, kForgetGateBiasTensor);
  const TfLiteTensor* cell_gate_bias =
      GetInput(context, node, kCellGateBiasTensor);
  const TfLiteTensor* output_gate_bias =
      GetInput(context, node, kOutputGateBiasTensor);

  const TfLiteTensor* projection_weights =
      GetOptionalInputTensor(context, node, kProjectionWeightsTensor);
  const TfLiteTensor* projection_bias =
      GetOptionalInputTensor(context, node, kProjectionBiasTensor);

  TfLiteTensor* output_state =
      GetVariableInput(context, node, kOutputStateTensor);
  TF_LITE_ENSURE(context, output_state != nullptr);
  TfLiteTensor* cell_state = GetVariableInput(context, node, kCellStateTensor);
  TF_LITE_ENSURE(context, cell_state != nullptr);

  // Since we have already checked that weights are all there or none, we can
  // check the existence of only one to get the condition.
  const bool use_cifg = (input_to_input_weights == nullptr);
  const bool use_peephole = (cell_to_output_weights != nullptr);
  const bool is_layer_norm_lstm = (forget_layer_norm_coefficients != nullptr);
  const bool use_projection = (projection_weights != nullptr);

  // Weights and states.
  int8_t* input_to_input_weight_ptr = nullptr;
  int8_t* recurrent_to_input_weight_ptr = nullptr;
  int8_t* cell_to_input_weight_ptr = nullptr;
  int8_t* input_to_forget_weight_ptr = nullptr;
  int8_t* recurrent_to_forget_weight_ptr = nullptr;
  int8_t* cell_to_forget_weight_ptr = nullptr;
  int8_t* input_to_cell_weight_ptr = nullptr;
  int8_t* recurrent_to_cell_weight_ptr = nullptr;
  int8_t* input_to_output_weight_ptr = nullptr;
  int8_t* recurrent_to_output_weight_ptr = nullptr;
  int8_t* cell_to_output_weight_ptr = nullptr;
  int8_t* projection_weight_ptr = nullptr;
  int16_t* layer_norm_input_weight_ptr = nullptr;
  int16_t* layer_norm_forget_weight_ptr = nullptr;
  int16_t* layer_norm_cell_weight_ptr = nullptr;
  int16_t* layer_norm_output_weight_ptr = nullptr;
  int32_t* input_gate_bias_ptr = nullptr;
  int32_t* forget_gate_bias_ptr = nullptr;
  int32_t* cell_gate_bias_ptr = nullptr;
  int32_t* output_gate_bias_ptr = nullptr;
  int32_t* projection_bias_ptr = nullptr;
  int16_t* cell_ptr = nullptr;
  int8_t* output_state_ptr = nullptr;

  // Scales.
  const float default_scale = 1.0;
  float input_scale = default_scale;
  float input_to_input_weight_scale = default_scale;
  float recurrent_to_input_weight_scale = default_scale;
  float cell_to_input_weight_scale = default_scale;
  float input_to_forget_weight_scale = default_scale;
  float recurrent_to_forget_weight_scale = default_scale;
  float cell_to_forget_weight_scale = default_scale;
  float input_to_cell_weight_scale = default_scale;
  float recurrent_to_cell_weight_scale = default_scale;
  float input_to_output_weight_scale = default_scale;
  float recurrent_to_output_weight_scale = default_scale;
  float cell_to_output_weight_scale = default_scale;
  float projection_weight_scale = default_scale;
  float layer_norm_input_scale = default_scale;
  float layer_norm_forget_scale = default_scale;
  float layer_norm_cell_scale = default_scale;
  float layer_norm_output_scale = default_scale;
  float output_state_scale = default_scale;

  // Effective scales.
  float effective_input_to_input_scale = default_scale;
  float effective_recurrent_to_input_scale = default_scale;
  float effective_cell_to_input_scale = default_scale;
  float effective_input_to_forget_scale = default_scale;
  float effective_recurrent_to_forget_scale = default_scale;
  float effective_cell_to_forget_scale = default_scale;
  float effective_input_to_cell_scale = default_scale;
  float effective_recurrent_to_cell_scale = default_scale;
  float effective_input_to_output_scale = default_scale;
  float effective_recurrent_to_output_scale = default_scale;
  float effective_cell_to_output_scale = default_scale;
  float effective_proj_scale = default_scale;

  // Zero points
  int input_zp = 0;
  int output_state_zp = 0;

  // Populate all the values.
  if (!use_cifg) {
    input_to_input_weight_ptr = input_to_input_weights->data.int8;
    recurrent_to_input_weight_ptr = recurrent_to_input_weights->data.int8;
    input_gate_bias_ptr = input_gate_bias->data.i32;
    input_to_input_weight_scale = input_to_input_weights->params.scale;
    recurrent_to_input_weight_scale = recurrent_to_input_weights->params.scale;
  }

  if (use_peephole) {
    if (!use_cifg) {
      cell_to_input_weight_ptr = cell_to_input_weights->data.int8;
      cell_to_input_weight_scale = cell_to_input_weights->params.scale;
    }
    cell_to_forget_weight_ptr = cell_to_forget_weights->data.int8;
    cell_to_output_weight_ptr = cell_to_output_weights->data.int8;
    cell_to_forget_weight_scale = cell_to_forget_weights->params.scale;
    cell_to_output_weight_scale = cell_to_output_weights->params.scale;
  }

  if (is_layer_norm_lstm) {
    if (!use_cifg) {
      layer_norm_input_weight_ptr = input_layer_norm_coefficients->data.i16;
      layer_norm_input_scale = input_layer_norm_coefficients->params.scale;
    }
    layer_norm_forget_weight_ptr = forget_layer_norm_coefficients->data.i16;
    layer_norm_forget_scale = forget_layer_norm_coefficients->params.scale;
    layer_norm_cell_weight_ptr = cell_layer_norm_coefficients->data.i16;
    layer_norm_cell_scale = cell_layer_norm_coefficients->params.scale;
    layer_norm_output_weight_ptr = output_layer_norm_coefficients->data.i16;
    layer_norm_output_scale = output_layer_norm_coefficients->params.scale;
  }

  if (use_projection) {
    projection_weight_ptr = projection_weights->data.int8;
    projection_weight_scale = projection_weights->params.scale;
    if (projection_bias) {
      projection_bias_ptr = projection_bias->data.i32;
    }
  }
  output_state_scale = output_state->params.scale;

  input_to_forget_weight_ptr = input_to_forget_weights->data.int8;
  input_to_forget_weight_scale = input_to_forget_weights->params.scale;
  input_to_cell_weight_ptr = input_to_cell_weights->data.int8;
  input_to_cell_weight_scale = input_to_cell_weights->params.scale;
  input_to_output_weight_ptr = input_to_output_weights->data.int8;
  input_to_output_weight_scale = input_to_output_weights->params.scale;
  recurrent_to_forget_weight_ptr = recurrent_to_forget_weights->data.int8;
  recurrent_to_forget_weight_scale = recurrent_to_forget_weights->params.scale;
  recurrent_to_cell_weight_ptr = recurrent_to_cell_weights->data.int8;
  recurrent_to_cell_weight_scale = recurrent_to_cell_weights->params.scale;
  recurrent_to_output_weight_ptr = recurrent_to_output_weights->data.int8;
  recurrent_to_output_weight_scale = recurrent_to_output_weights->params.scale;
  forget_gate_bias_ptr = forget_gate_bias->data.i32;
  cell_gate_bias_ptr = cell_gate_bias->data.i32;
  output_gate_bias_ptr = output_gate_bias->data.i32;
  output_state_ptr = output_state->data.int8;
  cell_ptr = cell_state->data.i16;
  input_scale = input->params.scale;
  input_zp = input->params.zero_point;
  output_state_zp = output_state->params.zero_point;

  std::vector<float> intermediate_scale;
  for (int i = 0; i < 12; ++i) {
    TfLiteTensor* intermediate =
        &context->tensors[node->intermediates->data[i]];
    auto* params = reinterpret_cast<TfLiteAffineQuantization*>(
        intermediate->quantization.params);
    intermediate_scale.push_back(params->scale->data[0]);
    integer_lstm_param->intermediate_zp[i] = params->zero_point->data[0];
  }

  // Calculate effective scales.
  if (!use_cifg) {
    effective_input_to_input_scale =
        input_to_input_weight_scale * input_scale / intermediate_scale[1];
    effective_recurrent_to_input_scale = recurrent_to_input_weight_scale *
                                         output_state_scale /
                                         intermediate_scale[2];
  }
  effective_input_to_forget_scale =
      input_to_forget_weight_scale * input_scale / intermediate_scale[4];
  effective_recurrent_to_forget_scale = recurrent_to_forget_weight_scale *
                                        output_state_scale /
                                        intermediate_scale[5];

  effective_input_to_cell_scale =
      input_to_cell_weight_scale * input_scale / intermediate_scale[7];
  effective_recurrent_to_cell_scale = recurrent_to_cell_weight_scale *
                                      output_state_scale /
                                      intermediate_scale[8];

  effective_input_to_output_scale =
      input_to_output_weight_scale * input_scale / intermediate_scale[10];
  effective_recurrent_to_output_scale = recurrent_to_output_weight_scale *
                                        output_state_scale /
                                        intermediate_scale[11];
  effective_proj_scale =
      projection_weight_scale * std::pow(2, -15) / output_state_scale;

  if (use_peephole) {
    if (!use_cifg) {
      effective_cell_to_input_scale =
          std::pow(2, -15) * cell_to_input_weight_scale / intermediate_scale[0];
    }
    effective_cell_to_forget_scale =
        std::pow(2, -15) * cell_to_forget_weight_scale / intermediate_scale[3];
    effective_cell_to_output_scale =
        std::pow(2, -15) * cell_to_output_weight_scale / intermediate_scale[9];
  }

  // Calculate effecgive scales.
  QuantizeMultiplier(effective_input_to_input_scale,
                     &integer_lstm_param->effective_input_to_input_scale_a,
                     &integer_lstm_param->effective_input_to_input_scale_b);
  QuantizeMultiplier(effective_recurrent_to_input_scale,
                     &integer_lstm_param->effective_recurrent_to_input_scale_a,
                     &integer_lstm_param->effective_recurrent_to_input_scale_b);
  QuantizeMultiplier(effective_cell_to_input_scale,
                     &integer_lstm_param->effective_cell_to_input_scale_a,
                     &integer_lstm_param->effective_cell_to_input_scale_b);
  QuantizeMultiplier(effective_input_to_forget_scale,
                     &integer_lstm_param->effective_input_to_forget_scale_a,
                     &integer_lstm_param->effective_input_to_forget_scale_b);
  QuantizeMultiplier(
      effective_recurrent_to_forget_scale,
      &integer_lstm_param->effective_recurrent_to_forget_scale_a,
      &integer_lstm_param->effective_recurrent_to_forget_scale_b);
  QuantizeMultiplier(effective_cell_to_forget_scale,
                     &integer_lstm_param->effective_cell_to_forget_scale_a,
                     &integer_lstm_param->effective_cell_to_forget_scale_b);
  QuantizeMultiplier(effective_input_to_cell_scale,
                     &integer_lstm_param->effective_input_to_cell_scale_a,
                     &integer_lstm_param->effective_input_to_cell_scale_b);
  QuantizeMultiplier(effective_recurrent_to_cell_scale,
                     &integer_lstm_param->effective_recurrent_to_cell_scale_a,
                     &integer_lstm_param->effective_recurrent_to_cell_scale_b);
  QuantizeMultiplier(effective_input_to_output_scale,
                     &integer_lstm_param->effective_input_to_output_scale_a,
                     &integer_lstm_param->effective_input_to_output_scale_b);
  QuantizeMultiplier(
      effective_recurrent_to_output_scale,
      &integer_lstm_param->effective_recurrent_to_output_scale_a,
      &integer_lstm_param->effective_recurrent_to_output_scale_b);
  QuantizeMultiplier(effective_cell_to_output_scale,
                     &integer_lstm_param->effective_cell_to_output_scale_a,
                     &integer_lstm_param->effective_cell_to_output_scale_b);
  QuantizeMultiplier(effective_proj_scale,
                     &integer_lstm_param->effective_proj_scale_a,
                     &integer_lstm_param->effective_proj_scale_b);
  QuantizeMultiplier(layer_norm_input_scale,
                     &integer_lstm_param->layer_norm_input_scale_a,
                     &integer_lstm_param->layer_norm_input_scale_b);
  QuantizeMultiplier(layer_norm_forget_scale,
                     &integer_lstm_param->layer_norm_forget_scale_a,
                     &integer_lstm_param->layer_norm_forget_scale_b);
  QuantizeMultiplier(layer_norm_cell_scale,
                     &integer_lstm_param->layer_norm_cell_scale_a,
                     &integer_lstm_param->layer_norm_cell_scale_b);
  QuantizeMultiplier(layer_norm_output_scale,
                     &integer_lstm_param->layer_norm_output_scale_a,
                     &integer_lstm_param->layer_norm_output_scale_b);

  {
    // Intermdiates in flatbuffer holds Wx, Wh and Wx+Wh.
    // effective Wx, Wh is in effective_input/recurrent_to_<...>_scale
    // So use intermediate_scale to hold scale from Wx and Wh to Wx+Wh
    // 0: [1] -> [0]
    // 1: [2] -> [0]
    // and use intermdiate_zp as is.
    const float s_1_0 = intermediate_scale[1] / intermediate_scale[0];
    const float s_2_0 = intermediate_scale[2] / intermediate_scale[0];
    const float s_4_3 = intermediate_scale[4] / intermediate_scale[3];
    const float s_5_3 = intermediate_scale[5] / intermediate_scale[3];
    const float s_7_6 = intermediate_scale[7] / intermediate_scale[6];
    const float s_8_6 = intermediate_scale[8] / intermediate_scale[6];
    const float s_10_9 = intermediate_scale[10] / intermediate_scale[9];
    const float s_11_9 = intermediate_scale[11] / intermediate_scale[9];
    QuantizeMultiplier(s_1_0, &integer_lstm_param->intermediate_scale_a[0],
                       &integer_lstm_param->intermediate_scale_b[0]);
    QuantizeMultiplier(s_2_0, &integer_lstm_param->intermediate_scale_a[1],
                       &integer_lstm_param->intermediate_scale_b[1]);
    QuantizeMultiplier(s_4_3, &integer_lstm_param->intermediate_scale_a[2],
                       &integer_lstm_param->intermediate_scale_b[2]);
    QuantizeMultiplier(s_5_3, &integer_lstm_param->intermediate_scale_a[3],
                       &integer_lstm_param->intermediate_scale_b[3]);
    QuantizeMultiplier(s_7_6, &integer_lstm_param->intermediate_scale_a[4],
                       &integer_lstm_param->intermediate_scale_b[4]);
    QuantizeMultiplier(s_8_6, &integer_lstm_param->intermediate_scale_a[5],
                       &integer_lstm_param->intermediate_scale_b[5]);
    QuantizeMultiplier(s_10_9, &integer_lstm_param->intermediate_scale_a[6],
                       &integer_lstm_param->intermediate_scale_b[6]);
    QuantizeMultiplier(s_11_9, &integer_lstm_param->intermediate_scale_a[7],
                       &integer_lstm_param->intermediate_scale_b[7]);
  }

  // Calculate quantized clip for projection and cell.
  const auto* params = reinterpret_cast<TfLiteLSTMParams*>(node->builtin_data);
  const float cell_clip = params->cell_clip;
  const float proj_clip = params->proj_clip;

  const TfLiteTensor* output_tensor = GetOutput(context, node, kOutputTensor);

  auto* cell_state_params = reinterpret_cast<TfLiteAffineQuantization*>(
      cell_state->quantization.params);
  auto* proj_params = reinterpret_cast<TfLiteAffineQuantization*>(
      output_tensor->quantization.params);
  TF_LITE_ENSURE_EQ(context, cell_state_params->scale->data[0], 1.0 / 32768);
  if (cell_clip > 0.0 && cell_clip < 1.0) {
    integer_lstm_param->quantized_cell_clip = static_cast<int16_t>(std::min(
        std::max(cell_clip / cell_state_params->scale->data[0], -32768.0f),
        32767.0f));
  } else {
    integer_lstm_param->quantized_cell_clip = 0;
  }
  if (proj_clip > 0.0) {
    integer_lstm_param->quantized_proj_clip = static_cast<int8_t>(std::min(
        std::max(proj_clip / proj_params->scale->data[0], -128.0f), 127.0f));
  } else {
    integer_lstm_param->quantized_proj_clip = 0;
  }
  return kTfLiteOk;
}