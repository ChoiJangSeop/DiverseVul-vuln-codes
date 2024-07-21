TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const auto* params =
      reinterpret_cast<TfLiteUnidirectionalSequenceLSTMParams*>(
          node->builtin_data);
  const OpData* op_data = reinterpret_cast<OpData*>(node->user_data);
  const bool is_layer_norm_lstm = op_data->is_layer_norm_lstm;
  const bool time_major = params->time_major;
  const TfLiteTensor* input = GetInput(context, node, lstm::full::kInputTensor);

  const TfLiteTensor* input_to_input_weights = GetOptionalInputTensor(
      context, node, lstm::full::kInputToInputWeightsTensor);
  const TfLiteTensor* input_to_forget_weights =
      GetInput(context, node, lstm::full::kInputToForgetWeightsTensor);
  const TfLiteTensor* input_to_cell_weights =
      GetInput(context, node, lstm::full::kInputToCellWeightsTensor);
  const TfLiteTensor* input_to_output_weights =
      GetInput(context, node, lstm::full::kInputToOutputWeightsTensor);

  const TfLiteTensor* recurrent_to_input_weights = GetOptionalInputTensor(
      context, node, lstm::full::kRecurrentToInputWeightsTensor);
  const TfLiteTensor* recurrent_to_forget_weights =
      GetInput(context, node, lstm::full::kRecurrentToForgetWeightsTensor);
  const TfLiteTensor* recurrent_to_cell_weights =
      GetInput(context, node, lstm::full::kRecurrentToCellWeightsTensor);
  const TfLiteTensor* recurrent_to_output_weights =
      GetInput(context, node, lstm::full::kRecurrentToOutputWeightsTensor);

  const TfLiteTensor* cell_to_input_weights = GetOptionalInputTensor(
      context, node, lstm::full::kCellToInputWeightsTensor);
  const TfLiteTensor* cell_to_forget_weights = GetOptionalInputTensor(
      context, node, lstm::full::kCellToForgetWeightsTensor);
  const TfLiteTensor* cell_to_output_weights = GetOptionalInputTensor(
      context, node, lstm::full::kCellToOutputWeightsTensor);

  const TfLiteTensor* input_gate_bias =
      GetOptionalInputTensor(context, node, lstm::full::kInputGateBiasTensor);
  const TfLiteTensor* forget_gate_bias =
      GetInput(context, node, lstm::full::kForgetGateBiasTensor);
  const TfLiteTensor* cell_gate_bias =
      GetInput(context, node, lstm::full::kCellGateBiasTensor);
  const TfLiteTensor* output_gate_bias =
      GetInput(context, node, lstm::full::kOutputGateBiasTensor);

  const TfLiteTensor* projection_weights = GetOptionalInputTensor(
      context, node, lstm::full::kProjectionWeightsTensor);
  const TfLiteTensor* projection_bias =
      GetOptionalInputTensor(context, node, lstm::full::kProjectionBiasTensor);

  // Index the scratch buffers pointers to the global scratch buffer.
  TfLiteTensor* scratch_buffer = GetTemporary(context, node, kScratchBuffer);

  TfLiteTensor* output_state =
      GetVariableInput(context, node, lstm::full::kOutputStateTensor);
  TF_LITE_ENSURE(context, output_state != nullptr);
  TfLiteTensor* cell_state =
      GetVariableInput(context, node, lstm::full::kCellStateTensor);
  TF_LITE_ENSURE(context, cell_state != nullptr);

  const TfLiteTensor* input_layer_norm_coefficients =
      is_layer_norm_lstm
          ? GetOptionalInputTensor(
                context, node, lstm::full::kInputLayerNormCoefficientsTensor)
          : nullptr;
  const TfLiteTensor* forget_layer_norm_coefficients =
      is_layer_norm_lstm
          ? GetInput(context, node,
                     lstm::full::kForgetLayerNormCoefficientsTensor)
          : nullptr;
  const TfLiteTensor* cell_layer_norm_coefficients =
      is_layer_norm_lstm
          ? GetInput(context, node,
                     lstm::full::kCellLayerNormCoefficientsTensor)
          : nullptr;
  const TfLiteTensor* output_layer_norm_coefficients =
      is_layer_norm_lstm
          ? GetInput(context, node,
                     lstm::full::kOutputLayerNormCoefficientsTensor)
          : nullptr;

  TfLiteTensor* output = GetOutput(context, node, lstm::full::kOutputTensor);

  // Copy out the LSTM specific params so they can be passed in the function.
  TfLiteLSTMParams lstm_params;
  lstm_params.activation = params->activation;
  lstm_params.cell_clip = params->cell_clip;
  lstm_params.proj_clip = params->proj_clip;
  lstm_params.asymmetric_quantize_inputs = params->asymmetric_quantize_inputs;

  switch (input_to_output_weights->type) {
    case kTfLiteFloat32: {
      return lstm_eval::EvalFloat(
          input, input_to_input_weights, input_to_forget_weights,
          input_to_cell_weights, input_to_output_weights,
          recurrent_to_input_weights, recurrent_to_forget_weights,
          recurrent_to_cell_weights, recurrent_to_output_weights,
          cell_to_input_weights, cell_to_forget_weights, cell_to_output_weights,
          input_layer_norm_coefficients, forget_layer_norm_coefficients,
          cell_layer_norm_coefficients, output_layer_norm_coefficients,
          /*aux_input=*/nullptr,
          /*aux_input_to_input_weights=*/nullptr,
          /*aux_input_to_forget_weights=*/nullptr,
          /*aux_input_to_cell_weights=*/nullptr,
          /*aux_input_to_output_weights=*/nullptr, input_gate_bias,
          forget_gate_bias, cell_gate_bias, output_gate_bias,
          projection_weights, projection_bias, &lstm_params,
          /*forward_sequence=*/true, time_major,
          /*output_offset=*/0, scratch_buffer, output_state, cell_state,
          output);
    }
    case kTfLiteUInt8:
    case kTfLiteInt8: {
      OpData* op_data = reinterpret_cast<OpData*>(node->user_data);
      TfLiteTensor* row_sums = GetTemporary(context, node, kRowSums);
      const int row_sums_size = row_sums->dims->data[0];
      return lstm_eval::EvalHybrid(
          input, input_to_input_weights,
          /*input_to_input_weights_ledger*/ nullptr, input_to_forget_weights,
          /*input_to_forget_weights_ledger*/ nullptr, input_to_cell_weights,
          /*input_to_cell_weights_ledger*/ nullptr, input_to_output_weights,
          /*input_to_output_weights_ledger*/ nullptr,
          recurrent_to_input_weights,
          /*recurrent_to_input_weights_ledger*/ nullptr,
          recurrent_to_forget_weights,
          /*recurrent_to_forget_weights_ledger*/ nullptr,
          recurrent_to_cell_weights,
          /*recurrent_to_cell_weights_ledger*/ nullptr,
          recurrent_to_output_weights,
          /*recurrent_to_output_weights_ledger*/ nullptr, cell_to_input_weights,
          cell_to_forget_weights, cell_to_output_weights,
          input_layer_norm_coefficients, forget_layer_norm_coefficients,
          cell_layer_norm_coefficients, output_layer_norm_coefficients,
          /*aux_input=*/nullptr,
          /*aux_input_to_input_weights=*/nullptr,
          /*aux_input_to_forget_weights=*/nullptr,
          /*aux_input_to_cell_weights=*/nullptr,
          /*aux_input_to_output_weights=*/nullptr, input_gate_bias,
          forget_gate_bias, cell_gate_bias, output_gate_bias,
          projection_weights, /*projection_weights_ledger*/ nullptr,
          projection_bias, &lstm_params,
          /*forward_sequence=*/true, time_major,
          /*output_offset=*/0, scratch_buffer,
          GetTemporary(context, node, kInputScalingFactors),
          /*aux_input_sf=*/nullptr,
          GetTemporary(context, node, kOutputStateScalingFactors),
          GetTemporary(context, node, kProductScalingFactors),
          GetTemporary(context, node, kRecoveredCellWeights),
          GetTemporary(context, node, kInputQuantized),
          /*aux_input_quantized=*/nullptr,
          GetTemporary(context, node, kOutputStateQuantized),
          GetTemporary(context, node, kCellStateQuantized), output_state,
          cell_state, GetTemporary(context, node, kAccumScratch), output,
          GetTemporary(context, node, kInputZeroPoints),
          /*aux_input_zp=*/nullptr,
          GetTemporary(context, node, kOutputStateZeroPoints), row_sums,
          row_sums_size, &op_data->compute_row_sums,
          CpuBackendContext::GetFromContext(context));
    }
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s is not currently supported.",
                         TfLiteTypeGetName(input_to_output_weights->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}