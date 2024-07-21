TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  OpData* op_data = static_cast<OpData*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, node->outputs->size, 1);
  // Logic for determining regular lstm and layer norm lstm:
  // input_size, forget_gate_layer_norm_tensor (20) null? is_layer_norm?
  // 20,         N/A,                                     No.
  // 24,         null,                                    No.
  // 24,         not null,                                Yes.
  // 20-inputs lstm are deprecated and is only kept here for backward
  // compatibility.
  if (node->inputs->size == 24) {
    const TfLiteTensor* forget_layer_norm_coefficients = GetOptionalInputTensor(
        context, node, kForgetLayerNormCoefficientsTensor);
    if (forget_layer_norm_coefficients == nullptr) {
      op_data->use_layer_norm = false;
    } else {
      op_data->use_layer_norm = true;
    }
  } else if (node->inputs->size == 20) {
    // This is deprecated and is only kept here for backward compatibility.
    op_data->use_layer_norm = false;
  } else {
    context->ReportError(
        context, "The LSTM Full kernel expects 20 or 24 inputs. Got %d inputs",
        node->inputs->size);
    return kTfLiteError;
  }

  const bool use_layer_norm = op_data->use_layer_norm;

  // Inferring batch size, number of outputs and number of cells from the
  // input tensors.
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const bool is_integer = input->type == kTfLiteInt8;
  TF_LITE_ENSURE(context, input->dims->size > 1);
  const int n_batch = input->dims->data[0];
  const int n_input = input->dims->data[1];

  const TfLiteTensor* input_to_output_weights =
      GetInput(context, node, kInputToOutputWeightsTensor);
  const int n_cell = input_to_output_weights->dims->data[0];
  TF_LITE_ENSURE_EQ(context, input_to_output_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, input_to_output_weights->dims->data[1], n_input);

  const TfLiteTensor* recurrent_to_output_weights =
      GetInput(context, node, kRecurrentToOutputWeightsTensor);
  TF_LITE_ENSURE_EQ(context, recurrent_to_output_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, recurrent_to_output_weights->dims->data[0],
                    n_cell);
  const int n_output = recurrent_to_output_weights->dims->data[1];

  // Check that input tensor dimensions matches with each other.
  TF_LITE_ENSURE_OK(
      context, CheckInputTensorDimensions(context, node, n_input, n_output,
                                          n_cell, use_layer_norm, is_integer));

  // Get the pointer to output, output_state and cell_state tensors.
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  TfLiteTensor* output_state =
      GetVariableInput(context, node, kOutputStateTensor);
  TF_LITE_ENSURE(context, output_state != nullptr);
  TfLiteTensor* cell_state = GetVariableInput(context, node, kCellStateTensor);
  TF_LITE_ENSURE(context, cell_state != nullptr);

  // Check the shape of input state tensors.
  // These tensor may be 1D or 2D. It's fine as long as the total size is
  // correct.
  TF_LITE_ENSURE_EQ(context, NumElements(output_state), n_batch * n_output);
  TF_LITE_ENSURE_EQ(context, NumElements(cell_state), n_batch * n_cell);

  // Resize the output tensors.
  TfLiteIntArray* output_size = TfLiteIntArrayCreate(2);
  output_size->data[0] = n_batch;
  output_size->data[1] = n_output;
  TF_LITE_ENSURE_OK(context,
                    context->ResizeTensor(context, output, output_size));

  // The weights are of consistent type, so it suffices to check one.
  const bool is_hybrid_op = IsHybridOp(input, input_to_output_weights);

  const bool is_sparse_op = (input_to_output_weights->sparsity != nullptr);

  // The type of Integer LSTM.
  const int num_intermediate_tensors = node->intermediates->size;
  if (is_integer) {
    TF_LITE_ENSURE(context, num_intermediate_tensors == 5 ||
                                num_intermediate_tensors == 12);
  }
  // We use number of intermediate tensors to distinguish the 8 bit matmul
  // output and the 16 bit matmul output version.
  const bool is_8x8_16 = num_intermediate_tensors == 5;

  TfLiteIntArrayFree(node->temporaries);
  if (is_hybrid_op) {
    if (is_sparse_op) {
      node->temporaries =
          TfLiteIntArrayCreate(kNumHybridTemporaryTensors + kLedgersToAdd);
    } else {
      node->temporaries = TfLiteIntArrayCreate(kNumHybridTemporaryTensors);
    }
  } else if (is_integer) {
    if (is_8x8_16) {
      node->temporaries = TfLiteIntArrayCreate(6);
    } else {
      node->temporaries = TfLiteIntArrayCreate(8);
    }
  } else {
    node->temporaries = TfLiteIntArrayCreate(1);
  }

  // Create a scratch buffer tensor for float case and hybrid case.
  // TODO(b/152066492): Create a is_float boolean and reorganize the temporary
  // buffer allocation logic.
  if (!is_integer) {
    node->temporaries->data[kScratchBuffer] =
        op_data->scratch_tensor_index + kScratchBuffer;
    TfLiteTensor* scratch_buffer = GetTemporary(context, node, kScratchBuffer);
    scratch_buffer->type = input->type;
    scratch_buffer->allocation_type = kTfLiteArenaRw;

    const TfLiteTensor* input_to_input_weights =
        GetOptionalInputTensor(context, node, kInputToInputWeightsTensor);
    const bool use_cifg = (input_to_input_weights == nullptr);
    TfLiteIntArray* scratch_buffer_size = TfLiteIntArrayCreate(2);
    scratch_buffer_size->data[0] = n_batch;
    if (use_cifg) {
      // Reserving space for Cell, Forget, Output gates
      scratch_buffer_size->data[1] = n_cell * 3;
    } else {
      // Reserving space for Input, Cell, Forget, Output gates
      scratch_buffer_size->data[1] = n_cell * 4;
    }
    TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, scratch_buffer,
                                                     scratch_buffer_size));
  }

  if (is_hybrid_op) {
    if (!is_sparse_op) {
      op_data->compute_row_sums = true;
    }
    // Allocate temporary tensors to store quantized values of input,
    // output_state and cell_state tensors.
    node->temporaries->data[kInputQuantized] =
        op_data->scratch_tensor_index + kInputQuantized;
    TfLiteTensor* input_quantized =
        GetTemporary(context, node, kInputQuantized);
    input_quantized->type = input_to_output_weights->type;
    input_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(input_quantized->dims, input->dims)) {
      TfLiteIntArray* input_quantized_size = TfLiteIntArrayCopy(input->dims);
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, input_quantized,
                                                       input_quantized_size));
    }
    node->temporaries->data[kOutputStateQuantized] =
        op_data->scratch_tensor_index + kOutputStateQuantized;
    TfLiteTensor* output_state_quantized =
        GetTemporary(context, node, kOutputStateQuantized);
    output_state_quantized->type = input_to_output_weights->type;
    output_state_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(output_state_quantized->dims,
                             output_state->dims)) {
      TfLiteIntArray* output_state_quantized_size =
          TfLiteIntArrayCopy(output_state->dims);
      TF_LITE_ENSURE_OK(context,
                        context->ResizeTensor(context, output_state_quantized,
                                              output_state_quantized_size));
    }
    node->temporaries->data[kCellStateQuantized] =
        op_data->scratch_tensor_index + kCellStateQuantized;
    TfLiteTensor* cell_state_quantized =
        GetTemporary(context, node, kCellStateQuantized);
    cell_state_quantized->type = input_to_output_weights->type;
    cell_state_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(cell_state_quantized->dims, cell_state->dims)) {
      TfLiteIntArray* cell_state_quantized_size =
          TfLiteIntArrayCopy(cell_state->dims);
      TF_LITE_ENSURE_OK(context,
                        context->ResizeTensor(context, cell_state_quantized,
                                              cell_state_quantized_size));
    }
    // Allocate temporary tensors to store scaling factors and product scaling
    // factors. The latter is a convenience storage which allows to quantize
    // a vector once (which produces the scaling factors) and multiply it with
    // different matrices (which requires multiplying the scaling factors with
    // the scaling factor of the matrix).
    node->temporaries->data[kInputScalingFactors] =
        op_data->scratch_tensor_index + kInputScalingFactors;
    TfLiteTensor* input_sf = GetTemporary(context, node, kInputScalingFactors);
    input_sf->type = kTfLiteFloat32;
    input_sf->allocation_type = kTfLiteArenaRw;
    int scaling_dims[1] = {n_batch};
    if (!TfLiteIntArrayEqualsArray(input_sf->dims, 1, scaling_dims)) {
      TfLiteIntArray* input_sf_size = TfLiteIntArrayCreate(1);
      input_sf_size->data[0] = n_batch;
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, input_sf, input_sf_size));
    }
    node->temporaries->data[kOutputStateScalingFactors] =
        op_data->scratch_tensor_index + kOutputStateScalingFactors;
    TfLiteTensor* output_state_sf =
        GetTemporary(context, node, kOutputStateScalingFactors);
    output_state_sf->type = kTfLiteFloat32;
    output_state_sf->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqualsArray(output_state_sf->dims, 1, scaling_dims)) {
      TfLiteIntArray* output_state_sf_size = TfLiteIntArrayCreate(1);
      output_state_sf_size->data[0] = n_batch;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, output_state_sf,
                                                       output_state_sf_size));
    }
    node->temporaries->data[kProductScalingFactors] =
        op_data->scratch_tensor_index + kProductScalingFactors;
    TfLiteTensor* prod_scaling_factors =
        GetTemporary(context, node, kProductScalingFactors);
    prod_scaling_factors->type = kTfLiteFloat32;
    prod_scaling_factors->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqualsArray(prod_scaling_factors->dims, 1,
                                   scaling_dims)) {
      TfLiteIntArray* prod_scaling_factors_size = TfLiteIntArrayCreate(1);
      prod_scaling_factors_size->data[0] = n_batch;
      TF_LITE_ENSURE_OK(context,
                        context->ResizeTensor(context, prod_scaling_factors,
                                              prod_scaling_factors_size));
    }

    // Allocate a temporary tensor to store the recovered cell weights. Since
    // this is used for diagonal matrices, only need to store n_cell values.
    node->temporaries->data[kRecoveredCellWeights] =
        op_data->scratch_tensor_index + kRecoveredCellWeights;
    TfLiteTensor* recovered_cell_weights =
        GetTemporary(context, node, kRecoveredCellWeights);
    recovered_cell_weights->type = kTfLiteFloat32;
    recovered_cell_weights->allocation_type = kTfLiteArenaRw;
    int recovered_cell_dims[1] = {n_cell};
    if (!TfLiteIntArrayEqualsArray(recovered_cell_weights->dims, 1,
                                   recovered_cell_dims)) {
      TfLiteIntArray* recovered_cell_weights_size = TfLiteIntArrayCreate(1);
      recovered_cell_weights_size->data[0] = n_cell;
      TF_LITE_ENSURE_OK(context,
                        context->ResizeTensor(context, recovered_cell_weights,
                                              recovered_cell_weights_size));
    }
    // Allocate a temporary tensor to store accumulate values for matrix
    // multiplication before multiplication by scaling factor
    node->temporaries->data[kAccumScratch] =
        op_data->scratch_tensor_index + kAccumScratch;
    TfLiteTensor* accum_scratch = GetTemporary(context, node, kAccumScratch);
    accum_scratch->type = kTfLiteInt32;
    accum_scratch->allocation_type = kTfLiteArenaRw;
    int accum_scratch_dims[2] = {n_cell, n_batch};
    if (!TfLiteIntArrayEqualsArray(accum_scratch->dims, 2,
                                   accum_scratch_dims)) {
      TfLiteIntArray* accum_size = TfLiteIntArrayCreate(2);
      accum_size->data[0] = n_cell;
      accum_size->data[1] = n_batch;
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, accum_scratch, accum_size));
    }
    node->temporaries->data[kInputZeroPoints] =
        op_data->scratch_tensor_index + kInputZeroPoints;
    TfLiteTensor* input_zp = GetTemporary(context, node, kInputZeroPoints);
    input_zp->type = kTfLiteFloat32;
    input_zp->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqualsArray(input_zp->dims, 1, scaling_dims)) {
      TfLiteIntArray* input_zp_size = TfLiteIntArrayCreate(1);
      input_zp_size->data[0] = n_batch;
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, input_zp, input_zp_size));
    }
    node->temporaries->data[kOutputStateZeroPoints] =
        op_data->scratch_tensor_index + kOutputStateZeroPoints;
    TfLiteTensor* output_state_zp =
        GetTemporary(context, node, kOutputStateZeroPoints);
    output_state_zp->type = kTfLiteFloat32;
    output_state_zp->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqualsArray(output_state_zp->dims, 1, scaling_dims)) {
      TfLiteIntArray* output_state_zp_size = TfLiteIntArrayCreate(1);
      output_state_zp_size->data[0] = n_batch;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, output_state_zp,
                                                       output_state_zp_size));
    }

    node->temporaries->data[kRowSums] =
        op_data->scratch_tensor_index + kRowSums;
    const TfLiteTensor* input_to_input_weights =
        GetOptionalInputTensor(context, node, kInputToInputWeightsTensor);
    const bool use_cifg = (input_to_input_weights == nullptr);
    int row_sums_rows = use_cifg ? 6 : 8;
    const TfLiteTensor* projection_weights =
        GetOptionalInputTensor(context, node, kProjectionWeightsTensor);
    if (projection_weights != nullptr) {
      row_sums_rows += ceil(static_cast<float>(n_output) / n_cell);
    }

    TfLiteTensor* row_sums = GetTemporary(context, node, kRowSums);
    row_sums->type = kTfLiteInt32;
    row_sums->allocation_type = kTfLiteArenaRwPersistent;
    const int row_sums_dims[2] = {row_sums_rows, n_cell};
    if (!TfLiteIntArrayEqualsArray(row_sums->dims, 2, row_sums_dims)) {
      TfLiteIntArray* row_sums_size = TfLiteIntArrayCreate(2);
      row_sums_size->data[0] = row_sums_dims[0];
      row_sums_size->data[1] = row_sums_dims[1];
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, row_sums, row_sums_size));
    }

    if (is_sparse_op) {
      op_data->ledger_initialized = false;
      int offset = kNumHybridTemporaryTensors;
      {
        node->temporaries->data[offset + kInputToInputWeightsLedgerOffset] =
            op_data->ledger_index + kInputToInputWeightsLedgerOffset;
        const TfLiteTensor* input_to_input_weights =
            GetOptionalInputTensor(context, node, kInputToInputWeightsTensor);
        TfLiteTensor* input_to_input_weights_ledger =
            &context->tensors[op_data->ledger_index +
                              kInputToInputWeightsLedgerOffset];
        auto status = make_ledger(input_to_input_weights == nullptr
                                      ? nullptr
                                      : input_to_input_weights->sparsity,
                                  context, input_to_input_weights_ledger);
        if (status != kTfLiteOk) return status;
      }
      {
        node->temporaries->data[offset + kInputToForgetWeightsLedgerOffset] =
            op_data->ledger_index + kInputToForgetWeightsLedgerOffset;
        const TfLiteTensor* input_to_forget_weights =
            GetInput(context, node, kInputToForgetWeightsTensor);
        TfLiteTensor* input_to_forget_weights_ledger =
            &context->tensors[op_data->ledger_index +
                              kInputToForgetWeightsLedgerOffset];
        auto status = make_ledger(input_to_forget_weights->sparsity, context,
                                  input_to_forget_weights_ledger);
        if (status != kTfLiteOk) return status;
      }
      {
        node->temporaries->data[offset + kInputToCellWeightsLedgerOffset] =
            op_data->ledger_index + kInputToCellWeightsLedgerOffset;
        const TfLiteTensor* input_to_cell_weights =
            GetInput(context, node, kInputToCellWeightsTensor);
        TfLiteTensor* input_to_cell_weights_ledger =
            &context->tensors[op_data->ledger_index +
                              kInputToCellWeightsLedgerOffset];
        auto status = make_ledger(input_to_cell_weights->sparsity, context,
                                  input_to_cell_weights_ledger);
        if (status != kTfLiteOk) return status;
      }
      {
        node->temporaries->data[offset + kInputToOutputWeightsLedgerOffset] =
            op_data->ledger_index + kInputToOutputWeightsLedgerOffset;
        const TfLiteTensor* input_to_output_weights =
            GetInput(context, node, kInputToOutputWeightsTensor);
        TfLiteTensor* input_to_output_weights_ledger =
            &context->tensors[op_data->ledger_index +
                              kInputToOutputWeightsLedgerOffset];
        auto status = make_ledger(input_to_output_weights->sparsity, context,
                                  input_to_output_weights_ledger);
        if (status != kTfLiteOk) return status;
      }
      {
        node->temporaries->data[offset + kRecurrentToInputWeightsLedgerOffset] =
            op_data->ledger_index + kRecurrentToInputWeightsLedgerOffset;
        const TfLiteTensor* recurrent_to_input_weights = GetOptionalInputTensor(
            context, node, kRecurrentToInputWeightsTensor);
        TfLiteTensor* recurrent_to_input_weights_ledger =
            &context->tensors[op_data->ledger_index +
                              kRecurrentToInputWeightsLedgerOffset];
        auto status = make_ledger(recurrent_to_input_weights == nullptr
                                      ? nullptr
                                      : recurrent_to_input_weights->sparsity,
                                  context, recurrent_to_input_weights_ledger);
        if (status != kTfLiteOk) return status;
      }
      {
        node->temporaries
            ->data[offset + kRecurrentToForgetWeightsLedgerOffset] =
            op_data->ledger_index + kRecurrentToForgetWeightsLedgerOffset;
        const TfLiteTensor* recurrent_to_forget_weights =
            GetInput(context, node, kRecurrentToForgetWeightsTensor);
        TfLiteTensor* recurrent_to_forget_weights_ledger =
            &context->tensors[op_data->ledger_index +
                              kRecurrentToForgetWeightsLedgerOffset];
        auto status = make_ledger(recurrent_to_forget_weights->sparsity,
                                  context, recurrent_to_forget_weights_ledger);
        if (status != kTfLiteOk) return status;
      }
      {
        node->temporaries->data[offset + kRecurrentToCellWeightsLedgerOffset] =
            op_data->ledger_index + kRecurrentToCellWeightsLedgerOffset;
        const TfLiteTensor* recurrent_to_cell_weights =
            GetInput(context, node, kRecurrentToCellWeightsTensor);
        TfLiteTensor* recurrent_to_cell_weights_ledger =
            &context->tensors[op_data->ledger_index +
                              kRecurrentToCellWeightsLedgerOffset];
        auto status = make_ledger(recurrent_to_cell_weights->sparsity, context,
                                  recurrent_to_cell_weights_ledger);
        if (status != kTfLiteOk) return status;
      }
      {
        node->temporaries
            ->data[offset + kRecurrentToOutputWeightsLedgerOffset] =
            op_data->ledger_index + kRecurrentToOutputWeightsLedgerOffset;
        const TfLiteTensor* recurrent_to_output_weights =
            GetInput(context, node, kRecurrentToOutputWeightsTensor);
        TfLiteTensor* recurrent_to_output_weights_ledger =
            &context->tensors[op_data->ledger_index +
                              kRecurrentToOutputWeightsLedgerOffset];
        auto status = make_ledger(recurrent_to_output_weights->sparsity,
                                  context, recurrent_to_output_weights_ledger);
        if (status != kTfLiteOk) return status;
      }
      {
        node->temporaries->data[offset + kProjectionWeightsLedgerOffset] =
            op_data->ledger_index + kProjectionWeightsLedgerOffset;
        const TfLiteTensor* projection_weights =
            GetInput(context, node, kProjectionWeightsTensor);
        TfLiteTensor* projection_weights_ledger =
            &context->tensors[op_data->ledger_index +
                              kProjectionWeightsLedgerOffset];
        auto status = make_ledger(projection_weights->sparsity, context,
                                  projection_weights_ledger);
        if (status != kTfLiteOk) return status;
      }
    }
  }

  if (is_integer) {
    if (is_8x8_16) {
      // Integer LSTM prepare function for 8x8->16.
      // This code path needs 5 intermediate tensors per Op.
      // Populate quantization parameters.
      PopulateQuantizedLstmParams8x8_16(context, node,
                                        &op_data->integer_lstm_param);

      // Allocate scratch buffer. Need 6 16bit buffer with size n_batch * n_cell
      // and 1 8bit buffer with size n_batch * n_cell. We also need 1 32 bit
      // buffer with size n_batch * n_cell.
      //
      // Handle cifg case as well, which might save one buffer.
      for (int scratch_index = 0; scratch_index < 6; ++scratch_index) {
        node->temporaries->data[scratch_index] =
            op_data->scratch_tensor_index + scratch_index;
        TfLiteTensor* scratch_tensor =
            GetTemporary(context, node, scratch_index);
        scratch_tensor->type = kTfLiteInt16;
        if (scratch_index == 4) {
          scratch_tensor->type = kTfLiteInt8;
        } else if (scratch_index == 5) {
          scratch_tensor->type = kTfLiteInt32;
        }
        scratch_tensor->allocation_type = kTfLiteArenaRw;
        const int scratch_dimension[2] = {n_batch, n_cell};
        if (!TfLiteIntArrayEqualsArray(scratch_tensor->dims, 2,
                                       scratch_dimension)) {
          TfLiteIntArray* scratch_buffer_size = TfLiteIntArrayCreate(2);
          scratch_buffer_size->data[0] = n_batch;
          scratch_buffer_size->data[1] = n_cell;
          TF_LITE_ENSURE_OK(context,
                            context->ResizeTensor(context, scratch_tensor,
                                                  scratch_buffer_size));
        }
      }

      // Populate precomputed zp * weight.
      TF_LITE_ENSURE_OK(context, PopulatePrecomputedZPTimesWeightsWithBias(
                                     context, op_data, node));
    } else {
      // Integer LSTM prepare function for 8x8->8.
      // This code path needs 12 intermediate tensors per Op.
      PopulateQuantizedLstmParams8x8_8(context, node,
                                       &op_data->integer_lstm_param);

      // Allocate scratch buffer. Need 6 16bit buffer with size n_batch * n_cell
      // and 2 8bit buffer with size n_batch * n_cell.
      //
      // Handle cifg case as well, which might save one buffer.
      for (int scratch_index = 0; scratch_index < 8; ++scratch_index) {
        node->temporaries->data[scratch_index] =
            op_data->scratch_tensor_index + scratch_index;
        TfLiteTensor* scratch_tensor =
            GetTemporary(context, node, scratch_index);
        if (scratch_index == 0 || scratch_index == 1) {
          scratch_tensor->type = kTfLiteInt8;
        } else {
          scratch_tensor->type = kTfLiteInt16;
        }
        scratch_tensor->allocation_type = kTfLiteArenaRw;
        const int scratch_dimension[2] = {n_batch, n_cell};
        if (!TfLiteIntArrayEqualsArray(scratch_tensor->dims, 2,
                                       scratch_dimension)) {
          TfLiteIntArray* scratch_buffer_size = TfLiteIntArrayCreate(2);
          scratch_buffer_size->data[0] = n_batch;
          scratch_buffer_size->data[1] = n_cell;
          TF_LITE_ENSURE_OK(context,
                            context->ResizeTensor(context, scratch_tensor,
                                                  scratch_buffer_size));
        }
      }
    }
  }
  return kTfLiteOk;
}