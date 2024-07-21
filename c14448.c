TfLiteStatus PopulatePrecomputedZPTimesWeightsWithBias(TfLiteContext* context,
                                                       OpData* op_data,
                                                       TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* output_state =
      GetVariableInput(context, node, kOutputStateTensor);
  TF_LITE_ENSURE(context, output_state != nullptr);

  const int32_t input_zero_point = -input->params.zero_point;
  const int32_t output_state_zero_point = -output_state->params.zero_point;

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

  const TfLiteTensor* projection_weights =
      GetOptionalInputTensor(context, node, kProjectionWeightsTensor);
  const TfLiteTensor* projection_bias =
      GetOptionalInputTensor(context, node, kProjectionBiasTensor);

  lstm_eval::IntegerLstmParameter* integer_lstm_params =
      &op_data->integer_lstm_param;

  const TfLiteTensor* intermediate =
      &context->tensors[node->intermediates->data[4]];
  const auto* params =
      static_cast<TfLiteAffineQuantization*>(intermediate->quantization.params);
  const int32_t hidden_zp = params->zero_point->data[0];

  // Get bias and perform zero point calculation.
  // When there is layer normalization, the gate bias does not apply to matmul
  // directly:
  //      y = ln(w * x + w * r + w * c) + b.
  const bool is_layer_norm = op_data->use_layer_norm;

  // Forget gate.
  const TfLiteTensor* forget_gate_bias =
      is_layer_norm ? nullptr : GetInput(context, node, kForgetGateBiasTensor);
  TF_LITE_ENSURE_OK(
      context,
      PrecomputeZeroPointTimesWeightWithBias(
          context, input_zero_point, input_to_forget_weights, forget_gate_bias,
          &(integer_lstm_params->input_to_forget_effective_bias)));

  TF_LITE_ENSURE_OK(
      context,
      PrecomputeZeroPointTimesWeightWithBias(
          context, output_state_zero_point, recurrent_to_forget_weights,
          nullptr, &(integer_lstm_params->recurrent_to_forget_effective_bias)));

  // Modulation gate.
  const TfLiteTensor* cell_gate_bias =
      is_layer_norm ? nullptr : GetInput(context, node, kCellGateBiasTensor);
  TF_LITE_ENSURE_OK(
      context,
      PrecomputeZeroPointTimesWeightWithBias(
          context, input_zero_point, input_to_cell_weights, cell_gate_bias,
          &(integer_lstm_params->input_to_cell_effective_bias)));
  TF_LITE_ENSURE_OK(
      context,
      PrecomputeZeroPointTimesWeightWithBias(
          context, output_state_zero_point, recurrent_to_cell_weights, nullptr,
          &(integer_lstm_params->recurrent_to_cell_effective_bias)));

  // Output gate.
  const TfLiteTensor* output_gate_bias =
      is_layer_norm ? nullptr : GetInput(context, node, kOutputGateBiasTensor);
  TF_LITE_ENSURE_OK(
      context,
      PrecomputeZeroPointTimesWeightWithBias(
          context, input_zero_point, input_to_output_weights, output_gate_bias,
          &(integer_lstm_params->input_to_output_effective_bias)));

  TF_LITE_ENSURE_OK(
      context,
      PrecomputeZeroPointTimesWeightWithBias(
          context, output_state_zero_point, recurrent_to_output_weights,
          nullptr, &(integer_lstm_params->recurrent_to_output_effective_bias)));

  // Input gate. The calculation is only meaningful for non-cifg case.
  const TfLiteTensor* input_gate_bias =
      is_layer_norm ? nullptr : GetInput(context, node, kInputGateBiasTensor);
  TF_LITE_ENSURE_OK(
      context,
      PrecomputeZeroPointTimesWeightWithBias(
          context, input_zero_point, input_to_input_weights, input_gate_bias,
          &(integer_lstm_params->input_to_input_effective_bias)));
  TF_LITE_ENSURE_OK(
      context,
      PrecomputeZeroPointTimesWeightWithBias(
          context, output_state_zero_point, recurrent_to_input_weights, nullptr,
          &(integer_lstm_params->recurrent_to_input_effective_bias)));

  // Projection bias. The calculation is only meaningful for with projection.
  TF_LITE_ENSURE_OK(context,
                    PrecomputeZeroPointTimesWeightWithBias(
                        context, hidden_zp, projection_weights, projection_bias,
                        &(integer_lstm_params->projection_effective_bias)));
  return kTfLiteOk;
}