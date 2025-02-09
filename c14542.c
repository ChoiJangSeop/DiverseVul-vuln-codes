TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const auto* params = static_cast<TfLiteLSTMParams*>(node->builtin_data);
  OpData* op_data = static_cast<OpData*>(node->user_data);

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

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (input_to_output_weights->type) {
    case kTfLiteFloat32: {
      // Index the scratch buffers pointers to the global scratch buffer.
      TfLiteTensor* scratch_buffer = GetTemporary(context, node, 0);
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
          projection_weights, projection_bias, params,
          /*forward_sequence=*/true,
          /*time_major=*/true,
          /*output_offset=*/0, scratch_buffer, output_state, cell_state,
          output);
    }
    case kTfLiteUInt8:
    case kTfLiteInt8: {
      const bool is_hybrid = (input->type == kTfLiteFloat32);
      const bool is_sparse = input_to_output_weights->sparsity != nullptr;
      if (is_hybrid) {
        TfLiteTensor* row_sums = GetTemporary(context, node, kRowSums);
        const int row_sums_size = row_sums->dims->data[0];
        if (is_sparse) {
          TfLiteTensor* input_to_input_weights_ledger =
              &context->tensors[op_data->ledger_index +
                                kInputToInputWeightsLedgerOffset];
          TfLiteTensor* input_to_forget_weights_ledger =
              &context->tensors[op_data->ledger_index +
                                kInputToForgetWeightsLedgerOffset];
          TfLiteTensor* input_to_cell_weights_ledger =
              &context->tensors[op_data->ledger_index +
                                kInputToCellWeightsLedgerOffset];
          TfLiteTensor* input_to_output_weights_ledger =
              &context->tensors[op_data->ledger_index +
                                kInputToOutputWeightsLedgerOffset];
          TfLiteTensor* recurrent_to_input_weights_ledger =
              &context->tensors[op_data->ledger_index +
                                kRecurrentToInputWeightsLedgerOffset];
          TfLiteTensor* recurrent_to_forget_weights_ledger =
              &context->tensors[op_data->ledger_index +
                                kRecurrentToForgetWeightsLedgerOffset];
          TfLiteTensor* recurrent_to_cell_weights_ledger =
              &context->tensors[op_data->ledger_index +
                                kRecurrentToCellWeightsLedgerOffset];
          TfLiteTensor* recurrent_to_output_weights_ledger =
              &context->tensors[op_data->ledger_index +
                                kRecurrentToOutputWeightsLedgerOffset];
          TfLiteTensor* projection_weights_ledger =
              &context->tensors[op_data->ledger_index +
                                kProjectionWeightsLedgerOffset];
          if (!op_data->ledger_initialized) {
            copy_ledger(input_to_input_weights == nullptr
                            ? nullptr
                            : input_to_input_weights->sparsity,
                        input_to_input_weights_ledger);
            copy_ledger(input_to_forget_weights->sparsity,
                        input_to_forget_weights_ledger);
            copy_ledger(input_to_cell_weights->sparsity,
                        input_to_cell_weights_ledger);
            copy_ledger(input_to_output_weights->sparsity,
                        input_to_output_weights_ledger);
            copy_ledger(recurrent_to_input_weights == nullptr
                            ? nullptr
                            : recurrent_to_input_weights->sparsity,
                        recurrent_to_input_weights_ledger);
            copy_ledger(recurrent_to_forget_weights->sparsity,
                        recurrent_to_forget_weights_ledger);
            copy_ledger(recurrent_to_cell_weights->sparsity,
                        recurrent_to_cell_weights_ledger);
            copy_ledger(recurrent_to_output_weights->sparsity,
                        recurrent_to_output_weights_ledger);
            copy_ledger(projection_weights->sparsity,
                        projection_weights_ledger);
            op_data->ledger_initialized = true;
          }
          return lstm_eval::EvalHybrid(
              input, input_to_input_weights, input_to_input_weights_ledger,
              input_to_forget_weights, input_to_forget_weights_ledger,
              input_to_cell_weights, input_to_cell_weights_ledger,
              input_to_output_weights, input_to_output_weights_ledger,
              recurrent_to_input_weights, recurrent_to_input_weights_ledger,
              recurrent_to_forget_weights, recurrent_to_forget_weights_ledger,
              recurrent_to_cell_weights, recurrent_to_cell_weights_ledger,
              recurrent_to_output_weights, recurrent_to_output_weights_ledger,
              cell_to_input_weights, cell_to_forget_weights,
              cell_to_output_weights, input_layer_norm_coefficients,
              forget_layer_norm_coefficients, cell_layer_norm_coefficients,
              output_layer_norm_coefficients,
              /*aux_input=*/nullptr,
              /*aux_input_to_input_weights=*/nullptr,
              /*aux_input_to_forget_weights=*/nullptr,
              /*aux_input_to_cell_weights=*/nullptr,
              /*aux_input_to_output_weights=*/nullptr, input_gate_bias,
              forget_gate_bias, cell_gate_bias, output_gate_bias,
              projection_weights, projection_weights_ledger, projection_bias,
              params,
              /*forward_sequence=*/true, /*time_major=*/true,
              /*output_offset=*/0, GetTemporary(context, node, kScratchBuffer),
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
            /*recurrent_to_output_weights_ledger*/ nullptr,
            cell_to_input_weights, cell_to_forget_weights,
            cell_to_output_weights, input_layer_norm_coefficients,
            forget_layer_norm_coefficients, cell_layer_norm_coefficients,
            output_layer_norm_coefficients, /*aux_input=*/nullptr,
            /*aux_input_to_input_weights=*/nullptr,
            /*aux_input_to_forget_weights=*/nullptr,
            /*aux_input_to_cell_weights=*/nullptr,
            /*aux_input_to_output_weights=*/nullptr, input_gate_bias,
            forget_gate_bias, cell_gate_bias, output_gate_bias,
            projection_weights, /*projection_weights_ledger*/ nullptr,
            projection_bias, params,
            /*forward_sequence=*/true, /*time_major=*/true, /*output_offset=*/0,
            GetTemporary(context, node, kScratchBuffer),
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
      } else {
        const int num_intermediate_tensors = node->intermediates->size;
        if (num_intermediate_tensors == 5) {
          TfLiteTensor* scratch0 = GetTemporary(context, node, 0);
          TfLiteTensor* scratch1 = GetTemporary(context, node, 1);
          TfLiteTensor* scratch2 = GetTemporary(context, node, 2);
          TfLiteTensor* scratch3 = GetTemporary(context, node, 3);
          TfLiteTensor* scratch4 = GetTemporary(context, node, 4);
          TfLiteTensor* scratch5 = GetTemporary(context, node, 5);
          return lstm_eval::EvalInteger8x8_16(
              input, input_to_input_weights, input_to_forget_weights,
              input_to_cell_weights, input_to_output_weights,
              recurrent_to_input_weights, recurrent_to_forget_weights,
              recurrent_to_cell_weights, recurrent_to_output_weights,
              cell_to_input_weights, cell_to_forget_weights,
              cell_to_output_weights, input_layer_norm_coefficients,
              forget_layer_norm_coefficients, cell_layer_norm_coefficients,
              output_layer_norm_coefficients, input_gate_bias, forget_gate_bias,
              cell_gate_bias, output_gate_bias, projection_weights,
              projection_bias, params, &op_data->integer_lstm_param,
              output_state, cell_state, output, scratch0, scratch1, scratch2,
              scratch3, scratch4, scratch5,
              CpuBackendContext::GetFromContext(context));
        } else {
          TfLiteTensor* scratch0 = GetTemporary(context, node, 0);
          TfLiteTensor* scratch1 = GetTemporary(context, node, 1);
          TfLiteTensor* scratch2 = GetTemporary(context, node, 2);
          TfLiteTensor* scratch3 = GetTemporary(context, node, 3);
          TfLiteTensor* scratch4 = GetTemporary(context, node, 4);
          TfLiteTensor* scratch5 = GetTemporary(context, node, 5);
          TfLiteTensor* scratch6 = GetTemporary(context, node, 6);
          TfLiteTensor* scratch7 = GetTemporary(context, node, 7);
          return lstm_eval::EvalInteger8x8_8(
              input, input_to_input_weights, input_to_forget_weights,
              input_to_cell_weights, input_to_output_weights,
              recurrent_to_input_weights, recurrent_to_forget_weights,
              recurrent_to_cell_weights, recurrent_to_output_weights,
              cell_to_input_weights, cell_to_forget_weights,
              cell_to_output_weights, input_layer_norm_coefficients,
              forget_layer_norm_coefficients, cell_layer_norm_coefficients,
              output_layer_norm_coefficients, input_gate_bias, forget_gate_bias,
              cell_gate_bias, output_gate_bias, projection_weights,
              projection_bias, params, output_state, cell_state, output,
              &op_data->integer_lstm_param, scratch0, scratch1, scratch2,
              scratch3, scratch4, scratch5, scratch6, scratch7);
          return kTfLiteOk;
        }
      }
    }
    default:
      context->ReportError(context, "Type %d is not currently supported.",
                           input_to_output_weights->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}