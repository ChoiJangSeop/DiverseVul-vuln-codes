TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const auto* params = reinterpret_cast<TfLiteBidirectionalSequenceRNNParams*>(
      node->builtin_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* fw_input_weights =
      GetInput(context, node, kFwWeightsTensor);
  const TfLiteTensor* fw_recurrent_weights =
      GetInput(context, node, kFwRecurrentWeightsTensor);
  const TfLiteTensor* fw_bias = GetInput(context, node, kFwBiasTensor);
  const TfLiteTensor* bw_input_weights =
      GetInput(context, node, kBwWeightsTensor);
  const TfLiteTensor* bw_recurrent_weights =
      GetInput(context, node, kBwRecurrentWeightsTensor);
  const TfLiteTensor* bw_bias = GetInput(context, node, kBwBiasTensor);

  // Get auxiliary inputs.
  const TfLiteTensor* aux_input =
      GetOptionalInputTensor(context, node, kAuxInputTensor);
  const TfLiteTensor* fw_aux_input_weights =
      GetOptionalInputTensor(context, node, kFwAuxWeightsTensor);
  const TfLiteTensor* bw_aux_input_weights =
      GetOptionalInputTensor(context, node, kBwAuxWeightsTensor);

  TfLiteTensor* fw_hidden_state =
      GetVariableInput(context, node, kFwHiddenStateTensor);
  TF_LITE_ENSURE(context, fw_hidden_state != nullptr);
  TfLiteTensor* bw_hidden_state =
      GetVariableInput(context, node, kBwHiddenStateTensor);
  TF_LITE_ENSURE(context, bw_hidden_state != nullptr);

  TfLiteTensor* fw_output = GetOutput(context, node, kFwOutputTensor);
  TfLiteTensor* bw_output = params->merge_outputs
                                ? nullptr
                                : GetOutput(context, node, kBwOutputTensor);

  const bool has_previous_bw_output = (aux_input != nullptr);
  const bool use_aux_input = (fw_aux_input_weights != nullptr);

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

  switch (fw_input_weights->type) {
    case kTfLiteFloat32:
      return EvalFloat(input, bw_input, fw_input_weights, fw_recurrent_weights,
                       fw_bias, bw_input_weights, bw_recurrent_weights, bw_bias,
                       real_aux_input, fw_aux_input_weights,
                       bw_aux_input_weights, params, fw_hidden_state, fw_output,
                       bw_hidden_state, bw_output);
    case kTfLiteUInt8:
    case kTfLiteInt8: {
      TfLiteTensor* input_quantized =
          GetTemporary(context, node, kInputQuantized);
      TfLiteTensor* fw_hidden_state_quantized =
          GetTemporary(context, node, kFwHiddenStateQuantized);
      TfLiteTensor* bw_hidden_state_quantized =
          GetTemporary(context, node, kBwHiddenStateQuantized);
      TfLiteTensor* scaling_factors =
          GetTemporary(context, node, kScalingFactors);
      TfLiteTensor* zero_points = GetTemporary(context, node, kZeroPoints);
      TfLiteTensor* accum_scratch = GetTemporary(context, node, kAccumScratch);
      TfLiteTensor* fw_row_sums = GetTemporary(context, node, kFwRowSums);
      TfLiteTensor* bw_row_sums = GetTemporary(context, node, kBwRowSums);
      TfLiteTensor* aux_input_quantized =
          use_aux_input ? GetTemporary(context, node, kAuxInputQuantized)
                        : nullptr;
      auto* op_data = reinterpret_cast<OpData*>(node->user_data);
      return EvalHybrid(
          input, bw_input, fw_input_weights, fw_recurrent_weights, fw_bias,
          bw_input_weights, bw_recurrent_weights, bw_bias, real_aux_input,
          fw_aux_input_weights, bw_aux_input_weights, params, scaling_factors,
          input_quantized, aux_input_quantized, fw_hidden_state_quantized,
          fw_hidden_state, fw_output, bw_hidden_state_quantized,
          bw_hidden_state, bw_output, zero_points, accum_scratch, fw_row_sums,
          bw_row_sums, &op_data->fw_compute_row_sums,
          &op_data->bw_compute_row_sums);
    }
    default:
      context->ReportError(context, "Type not currently supported.");
      return kTfLiteError;
  }
  return kTfLiteOk;
}