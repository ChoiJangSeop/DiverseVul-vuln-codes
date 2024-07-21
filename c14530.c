TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const auto* params = reinterpret_cast<TfLiteBidirectionalSequenceLSTMParams*>(
      node->builtin_data);
  auto* op_data = reinterpret_cast<OpData*>(node->user_data);
  // Input tensor.
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);

  // Tensors for the forward cell.
  const TfLiteTensor* fw_input_to_input_weights =
      GetOptionalInputTensor(context, node, kFwInputToInputWeightsTensor);
  const TfLiteTensor* fw_input_to_forget_weights =
      GetInput(context, node, kFwInputToForgetWeightsTensor);
  const TfLiteTensor* fw_input_to_cell_weights =
      GetInput(context, node, kFwInputToCellWeightsTensor);
  const TfLiteTensor* fw_input_to_output_weights =
      GetInput(context, node, kFwInputToOutputWeightsTensor);

  const TfLiteTensor* fw_recurrent_to_input_weights =
      GetOptionalInputTensor(context, node, kFwRecurrentToInputWeightsTensor);
  const TfLiteTensor* fw_recurrent_to_forget_weights =
      GetInput(context, node, kFwRecurrentToForgetWeightsTensor);
  const TfLiteTensor* fw_recurrent_to_cell_weights =
      GetInput(context, node, kFwRecurrentToCellWeightsTensor);
  const TfLiteTensor* fw_recurrent_to_output_weights =
      GetInput(context, node, kFwRecurrentToOutputWeightsTensor);

  const TfLiteTensor* fw_cell_to_input_weights =
      GetOptionalInputTensor(context, node, kFwCellToInputWeightsTensor);
  const TfLiteTensor* fw_cell_to_forget_weights =
      GetOptionalInputTensor(context, node, kFwCellToForgetWeightsTensor);
  const TfLiteTensor* fw_cell_to_output_weights =
      GetOptionalInputTensor(context, node, kFwCellToOutputWeightsTensor);

  const TfLiteTensor* fw_input_gate_bias =
      GetOptionalInputTensor(context, node, kFwInputGateBiasTensor);
  const TfLiteTensor* fw_forget_gate_bias =
      GetInput(context, node, kFwForgetGateBiasTensor);
  const TfLiteTensor* fw_cell_gate_bias =
      GetInput(context, node, kFwCellGateBiasTensor);
  const TfLiteTensor* fw_output_gate_bias =
      GetInput(context, node, kFwOutputGateBiasTensor);

  const TfLiteTensor* fw_projection_weights =
      GetOptionalInputTensor(context, node, kFwProjectionWeightsTensor);
  const TfLiteTensor* fw_projection_bias =
      GetOptionalInputTensor(context, node, kFwProjectionBiasTensor);

  TfLiteTensor* fw_activation_state =
      GetVariableInput(context, node, kFwInputActivationStateTensor);
  TF_LITE_ENSURE(context, fw_activation_state != nullptr);
  TfLiteTensor* fw_cell_state =
      GetVariableInput(context, node, kFwInputCellStateTensor);
  TF_LITE_ENSURE(context, fw_cell_state != nullptr);
  TfLiteTensor* fw_output = GetOutput(context, node, kFwOutputTensor);

  // Tensors for the backward cell.
  const TfLiteTensor* bw_input_to_input_weights =
      GetOptionalInputTensor(context, node, kBwInputToInputWeightsTensor);
  const TfLiteTensor* bw_input_to_forget_weights =
      GetInput(context, node, kBwInputToForgetWeightsTensor);
  const TfLiteTensor* bw_input_to_cell_weights =
      GetInput(context, node, kBwInputToCellWeightsTensor);
  const TfLiteTensor* bw_input_to_output_weights =
      GetInput(context, node, kBwInputToOutputWeightsTensor);

  const TfLiteTensor* bw_recurrent_to_input_weights =
      GetOptionalInputTensor(context, node, kBwRecurrentToInputWeightsTensor);
  const TfLiteTensor* bw_recurrent_to_forget_weights =
      GetInput(context, node, kBwRecurrentToForgetWeightsTensor);
  const TfLiteTensor* bw_recurrent_to_cell_weights =
      GetInput(context, node, kBwRecurrentToCellWeightsTensor);
  const TfLiteTensor* bw_recurrent_to_output_weights =
      GetInput(context, node, kBwRecurrentToOutputWeightsTensor);

  const TfLiteTensor* bw_cell_to_input_weights =
      GetOptionalInputTensor(context, node, kBwCellToInputWeightsTensor);
  const TfLiteTensor* bw_cell_to_forget_weights =
      GetOptionalInputTensor(context, node, kBwCellToForgetWeightsTensor);
  const TfLiteTensor* bw_cell_to_output_weights =
      GetOptionalInputTensor(context, node, kBwCellToOutputWeightsTensor);

  const TfLiteTensor* bw_input_gate_bias =
      GetOptionalInputTensor(context, node, kBwInputGateBiasTensor);
  const TfLiteTensor* bw_forget_gate_bias =
      GetInput(context, node, kBwForgetGateBiasTensor);
  const TfLiteTensor* bw_cell_gate_bias =
      GetInput(context, node, kBwCellGateBiasTensor);
  const TfLiteTensor* bw_output_gate_bias =
      GetInput(context, node, kBwOutputGateBiasTensor);

  const TfLiteTensor* bw_projection_weights =
      GetOptionalInputTensor(context, node, kBwProjectionWeightsTensor);
  const TfLiteTensor* bw_projection_bias =
      GetOptionalInputTensor(context, node, kBwProjectionBiasTensor);

  // State tensors.
  TfLiteTensor* bw_activation_state =
      GetVariableInput(context, node, kBwInputActivationStateTensor);
  TF_LITE_ENSURE(context, bw_activation_state != nullptr);
  TfLiteTensor* bw_cell_state =
      GetVariableInput(context, node, kBwInputCellStateTensor);
  TF_LITE_ENSURE(context, bw_cell_state != nullptr);
  TfLiteTensor* bw_output = params->merge_outputs
                                ? nullptr
                                : GetOutput(context, node, kBwOutputTensor);

  // Temporary tensors.
  TfLiteTensor* fw_scratch_buffer =
      GetTemporary(context, node, kFwScratchBuffer);
  TfLiteTensor* bw_scratch_buffer =
      GetTemporary(context, node, kBwScratchBuffer);

  // (Optional) auxiliary inputs.
  const TfLiteTensor* aux_input =
      GetOptionalInputTensor(context, node, kAuxInputTensor);
  const TfLiteTensor* fw_aux_input_to_input_weights =
      GetOptionalInputTensor(context, node, kFwAuxInputToInputWeightsTensor);
  const TfLiteTensor* fw_aux_input_to_forget_weights =
      GetOptionalInputTensor(context, node, kFwAuxInputToForgetWeightsTensor);
  const TfLiteTensor* fw_aux_input_to_cell_weights =
      GetOptionalInputTensor(context, node, kFwAuxInputToCellWeightsTensor);
  const TfLiteTensor* fw_aux_input_to_output_weights =
      GetOptionalInputTensor(context, node, kFwAuxInputToOutputWeightsTensor);
  const TfLiteTensor* bw_aux_input_to_input_weights =
      GetOptionalInputTensor(context, node, kBwAuxInputToInputWeightsTensor);
  const TfLiteTensor* bw_aux_input_to_forget_weights =
      GetOptionalInputTensor(context, node, kBwAuxInputToForgetWeightsTensor);
  const TfLiteTensor* bw_aux_input_to_cell_weights =
      GetOptionalInputTensor(context, node, kBwAuxInputToCellWeightsTensor);
  const TfLiteTensor* bw_aux_input_to_output_weights =
      GetOptionalInputTensor(context, node, kBwAuxInputToOutputWeightsTensor);

  const bool has_previous_bw_output = (aux_input != nullptr);
  const bool use_aux_input = (fw_aux_input_to_forget_weights != nullptr);

  // Populate a TfLiteLSTMParams struct for the evaluation functions.
  TfLiteLSTMParams lstm_params = {params->activation, params->cell_clip,
                                  params->proj_clip, kTfLiteLSTMFullKernel,
                                  params->asymmetric_quantize_inputs};

  const int bw_output_offset =
      params->merge_outputs ? fw_recurrent_to_output_weights->dims->data[1] : 0;
  const auto actual_bw_output = params->merge_outputs ? fw_output : bw_output;

  const bool time_major = params->time_major;

  // We want to cover the following cases:
  //
  // If not stacking (not connected after other bidi lstms):
  //   both fw & bw will just use `input`; aux_input will be null.
  //
  // If stacking with cross_links, TensorFlow equivalent
  // (tf.contrib.rnn.stack_bidirectional_rnn):
  //   both fw & bw will use `input`, but aux_input will be none null.
  //   Note, this time, whether connected after other bidi lstms both works.
  //
  // If stacking without cross_links, but connected after other bidi lstms,
  // TensorFlow equivalent (tf.nn.static_bidirectional_rnn):
  //   fw will use `input`, bw will use aux_input, and the `real aux_input`
  //   will be null.

  const bool non_stacking_mode = !use_aux_input && has_previous_bw_output;
  const TfLiteTensor* bw_input = non_stacking_mode ? aux_input : input;
  const TfLiteTensor* real_aux_input = non_stacking_mode ? nullptr : aux_input;

  switch (fw_input_to_output_weights->type) {
    case kTfLiteFloat32: {
      TfLiteStatus fw_pass_status = lstm_eval::EvalFloat(
          input, fw_input_to_input_weights, fw_input_to_forget_weights,
          fw_input_to_cell_weights, fw_input_to_output_weights,
          fw_recurrent_to_input_weights, fw_recurrent_to_forget_weights,
          fw_recurrent_to_cell_weights, fw_recurrent_to_output_weights,
          fw_cell_to_input_weights, fw_cell_to_forget_weights,
          fw_cell_to_output_weights,
          /*input_layer_norm_coefficients=*/nullptr,
          /*forget_layer_norm_coefficients=*/nullptr,
          /*cell_layer_norm_coefficients=*/nullptr,
          /*output_layer_norm_coefficients=*/nullptr, real_aux_input,
          fw_aux_input_to_input_weights, fw_aux_input_to_forget_weights,
          fw_aux_input_to_cell_weights, fw_aux_input_to_output_weights,
          fw_input_gate_bias, fw_forget_gate_bias, fw_cell_gate_bias,
          fw_output_gate_bias, fw_projection_weights, fw_projection_bias,
          &lstm_params,
          /*forward_sequence=*/true, time_major, /*output_offset=*/0,
          fw_scratch_buffer, fw_activation_state, fw_cell_state, fw_output);
      TF_LITE_ENSURE_OK(context, fw_pass_status);

      TfLiteStatus bw_pass_status = lstm_eval::EvalFloat(
          bw_input, bw_input_to_input_weights, bw_input_to_forget_weights,
          bw_input_to_cell_weights, bw_input_to_output_weights,
          bw_recurrent_to_input_weights, bw_recurrent_to_forget_weights,
          bw_recurrent_to_cell_weights, bw_recurrent_to_output_weights,
          bw_cell_to_input_weights, bw_cell_to_forget_weights,
          bw_cell_to_output_weights,
          /*input_layer_norm_coefficients=*/nullptr,
          /*forget_layer_norm_coefficients=*/nullptr,
          /*cell_layer_norm_coefficients=*/nullptr,
          /*output_layer_norm_coefficients=*/nullptr, real_aux_input,
          bw_aux_input_to_input_weights, bw_aux_input_to_forget_weights,
          bw_aux_input_to_cell_weights, bw_aux_input_to_output_weights,
          bw_input_gate_bias, bw_forget_gate_bias, bw_cell_gate_bias,
          bw_output_gate_bias, bw_projection_weights, bw_projection_bias,
          &lstm_params,
          /*forward_sequence=*/false, time_major, bw_output_offset,
          bw_scratch_buffer, bw_activation_state, bw_cell_state,
          actual_bw_output);
      TF_LITE_ENSURE_OK(context, bw_pass_status);
      return kTfLiteOk;
    }
    case kTfLiteUInt8:
    case kTfLiteInt8: {
      TfLiteTensor* input_quantized =
          GetTemporary(context, node, kInputQuantized);
      TfLiteTensor* fw_activation_state_quantized =
          GetTemporary(context, node, kFwActivationStateQuantized);
      TfLiteTensor* bw_activation_state_quantized =
          GetTemporary(context, node, kBwActivationStateQuantized);
      TfLiteTensor* fw_cell_state_quantized =
          GetTemporary(context, node, kFwCellStateQuantized);
      TfLiteTensor* bw_cell_state_quantized =
          GetTemporary(context, node, kBwCellStateQuantized);
      TfLiteTensor* prod_scaling_factors =
          GetTemporary(context, node, kProductScalingFactors);
      TfLiteTensor* recovered_cell_weights =
          GetTemporary(context, node, kRecoveredCellWeights);
      TfLiteTensor* aux_input_quantized =
          use_aux_input ? GetTemporary(context, node, kAuxInputQuantized)
                        : nullptr;
      TfLiteTensor* accum_scratch =
          GetTemporary(context, node, kAccumScratchBuffer);
      TfLiteTensor* fw_row_sums = GetTemporary(context, node, kFwRowSums);
      TfLiteTensor* bw_row_sums = GetTemporary(context, node, kBwRowSums);
      const int fw_row_sums_size = fw_row_sums->dims->data[0];
      const int bw_row_sums_size = bw_row_sums->dims->data[0];
      TfLiteStatus fw_pass_status = lstm_eval::EvalHybrid(
          input, fw_input_to_input_weights,
          /*input_to_input_weights_ledger*/ nullptr, fw_input_to_forget_weights,
          /*input_to_forget_weights_ledger*/ nullptr, fw_input_to_cell_weights,
          /*input_to_cell_weights_ledger*/ nullptr, fw_input_to_output_weights,
          /*input_to_output_weights_ledger*/ nullptr,
          fw_recurrent_to_input_weights,
          /*recurrent_to_input_weights_ledger*/ nullptr,
          fw_recurrent_to_forget_weights,
          /*recurrent_to_forget_weights_ledger*/ nullptr,
          fw_recurrent_to_cell_weights,
          /*recurrent_to_cell_weights_ledger*/ nullptr,
          fw_recurrent_to_output_weights,
          /*recurrent_to_output_weights_ledger*/ nullptr,
          fw_cell_to_input_weights, fw_cell_to_forget_weights,
          fw_cell_to_output_weights,
          /*input_layer_norm_coefficients=*/nullptr,
          /*forget_layer_norm_coefficients=*/nullptr,
          /*cell_layer_norm_coefficients=*/nullptr,
          /*output_layer_norm_coefficients=*/nullptr, real_aux_input,
          fw_aux_input_to_input_weights, fw_aux_input_to_forget_weights,
          fw_aux_input_to_cell_weights, fw_aux_input_to_output_weights,
          fw_input_gate_bias, fw_forget_gate_bias, fw_cell_gate_bias,
          fw_output_gate_bias, fw_projection_weights,
          /*projection_weights_ledger*/ nullptr, fw_projection_bias,
          &lstm_params,
          /*forward_sequence=*/true, /*time_major=*/true, /*output_offset=*/0,
          fw_scratch_buffer, GetTemporary(context, node, kInputScalingFactors),
          GetTemporary(context, node, kAuxInputScalingFactors),
          GetTemporary(context, node, kOutputStateScalingFactors),
          prod_scaling_factors, recovered_cell_weights, input_quantized,
          aux_input_quantized, fw_activation_state_quantized,
          fw_cell_state_quantized, fw_activation_state, fw_cell_state,
          accum_scratch, fw_output,
          GetTemporary(context, node, kInputZeroPoints),
          GetTemporary(context, node, kAuxInputZeroPoints),
          GetTemporary(context, node, kOutputStateZeroPoints), fw_row_sums,
          fw_row_sums_size, &op_data->compute_fw_row_sums,
          CpuBackendContext::GetFromContext(context));
      TF_LITE_ENSURE_OK(context, fw_pass_status);

      TfLiteStatus bw_pass_status = lstm_eval::EvalHybrid(
          bw_input, bw_input_to_input_weights,
          /*input_to_input_weights_ledger*/ nullptr, bw_input_to_forget_weights,
          /*input_to_forget_weights_ledger*/ nullptr, bw_input_to_cell_weights,
          /*input_to_cell_weights_ledger*/ nullptr, bw_input_to_output_weights,
          /*input_to_output_weights_ledger*/ nullptr,
          bw_recurrent_to_input_weights,
          /*recurrent_to_input_weights_ledger*/ nullptr,
          bw_recurrent_to_forget_weights,
          /*recurrent_to_forget_weights_ledger*/ nullptr,
          bw_recurrent_to_cell_weights,
          /*recurrent_to_cell_weights_ledger*/ nullptr,
          bw_recurrent_to_output_weights,
          /*recurrent_to_output_weights_ledger*/ nullptr,
          bw_cell_to_input_weights, bw_cell_to_forget_weights,
          bw_cell_to_output_weights,
          /*input_layer_norm_coefficients=*/nullptr,
          /*forget_layer_norm_coefficients=*/nullptr,
          /*cell_layer_norm_coefficients=*/nullptr,
          /*output_layer_norm_coefficients=*/nullptr, real_aux_input,
          bw_aux_input_to_input_weights, bw_aux_input_to_forget_weights,
          bw_aux_input_to_cell_weights, bw_aux_input_to_output_weights,
          bw_input_gate_bias, bw_forget_gate_bias, bw_cell_gate_bias,
          bw_output_gate_bias, bw_projection_weights,
          /*projection_weights_ledger*/ nullptr, bw_projection_bias,
          &lstm_params,
          /*forward_sequence=*/false, /*time_major=*/true, bw_output_offset,
          bw_scratch_buffer, GetTemporary(context, node, kInputScalingFactors),
          GetTemporary(context, node, kAuxInputScalingFactors),
          GetTemporary(context, node, kOutputStateScalingFactors),
          prod_scaling_factors, recovered_cell_weights, input_quantized,
          aux_input_quantized, bw_activation_state_quantized,
          bw_cell_state_quantized, bw_activation_state, bw_cell_state,
          accum_scratch, actual_bw_output,
          GetTemporary(context, node, kInputZeroPoints),
          GetTemporary(context, node, kAuxInputZeroPoints),
          GetTemporary(context, node, kOutputStateZeroPoints), bw_row_sums,
          bw_row_sums_size, &op_data->compute_bw_row_sums,
          CpuBackendContext::GetFromContext(context));
      TF_LITE_ENSURE_OK(context, bw_pass_status);
      return kTfLiteOk;
    }
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s is not currently supported.",
                         TfLiteTypeGetName(fw_input_to_output_weights->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}