TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  auto* op_data = reinterpret_cast<OpData*>(node->user_data);
  const auto* params = reinterpret_cast<TfLiteBidirectionalSequenceLSTMParams*>(
      node->builtin_data);

  // Check we have all the inputs and outputs we need.
  TF_LITE_ENSURE_EQ(context, node->inputs->size, 48);
  TF_LITE_ENSURE_EQ(context, node->outputs->size,
                    params->merge_outputs ? 1 : 2);

  // Inferring batch size, number of outputs and sequence length and
  // number of cells from the input tensors.
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, kTfLiteFloat32);
  TF_LITE_ENSURE_EQ(context, input->dims->size, 3);
  const bool time_major = params->time_major;
  const int max_time = time_major ? input->dims->data[0] : input->dims->data[1];
  const int n_batch = time_major ? input->dims->data[1] : input->dims->data[0];
  const int n_input = input->dims->data[2];

  const TfLiteTensor* fw_input_to_output_weights =
      GetInput(context, node, kFwInputToOutputWeightsTensor);
  const int n_fw_cell = fw_input_to_output_weights->dims->data[0];
  TF_LITE_ENSURE_EQ(context, fw_input_to_output_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, fw_input_to_output_weights->dims->data[1],
                    n_input);

  const TfLiteTensor* bw_input_to_output_weights =
      GetInput(context, node, kBwInputToOutputWeightsTensor);
  const int n_bw_cell = bw_input_to_output_weights->dims->data[0];
  TF_LITE_ENSURE_EQ(context, bw_input_to_output_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, bw_input_to_output_weights->dims->data[1],
                    n_input);
  TF_LITE_ENSURE_EQ(context, bw_input_to_output_weights->type,
                    fw_input_to_output_weights->type);

  const TfLiteTensor* fw_recurrent_to_output_weights =
      GetInput(context, node, kFwRecurrentToOutputWeightsTensor);
  TF_LITE_ENSURE_EQ(context, fw_recurrent_to_output_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, fw_recurrent_to_output_weights->dims->data[0],
                    n_fw_cell);
  TF_LITE_ENSURE_EQ(context, fw_recurrent_to_output_weights->type,
                    fw_input_to_output_weights->type);
  const int n_fw_output = fw_recurrent_to_output_weights->dims->data[1];

  const TfLiteTensor* bw_recurrent_to_output_weights =
      GetInput(context, node, kBwRecurrentToOutputWeightsTensor);
  TF_LITE_ENSURE_EQ(context, bw_recurrent_to_output_weights->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, bw_recurrent_to_output_weights->dims->data[0],
                    n_bw_cell);
  TF_LITE_ENSURE_EQ(context, bw_recurrent_to_output_weights->type,
                    fw_input_to_output_weights->type);
  const int n_bw_output = bw_recurrent_to_output_weights->dims->data[1];

  // Check that input tensor dimensions matches with each other.
  TF_LITE_ENSURE_OK(
      context, CheckInputTensorDimensions(context, node, n_input, n_fw_output,
                                          n_fw_cell));

  // Get (optional) auxiliary inputs and weights.
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

  const bool aux_inputs_weights_all_or_none =
      ((fw_aux_input_to_cell_weights != nullptr) &&
       (fw_aux_input_to_forget_weights != nullptr) &&
       (fw_aux_input_to_output_weights != nullptr) &&
       (bw_aux_input_to_cell_weights != nullptr) &&
       (bw_aux_input_to_forget_weights != nullptr) &&
       (bw_aux_input_to_output_weights != nullptr)) ||
      ((fw_aux_input_to_cell_weights == nullptr) &&
       (fw_aux_input_to_forget_weights == nullptr) &&
       (fw_aux_input_to_output_weights == nullptr) &&
       (bw_aux_input_to_cell_weights == nullptr) &&
       (bw_aux_input_to_forget_weights == nullptr) &&
       (bw_aux_input_to_output_weights == nullptr));
  TF_LITE_ENSURE(context, aux_inputs_weights_all_or_none);

  const bool has_aux_input = (fw_aux_input_to_forget_weights != nullptr);

  if (has_aux_input) {
    // Check that aux_input has the same dimensions (except last) as the input.
    TF_LITE_ASSERT_EQ(aux_input->dims->data[0], input->dims->data[0]);
    TF_LITE_ASSERT_EQ(aux_input->dims->data[1], input->dims->data[1]);
  }

  // Get the pointer to output, activation_state and cell_state buffer tensors.
  TfLiteTensor* fw_output = GetOutput(context, node, kFwOutputTensor);
  TfLiteTensor* fw_activation_state =
      GetVariableInput(context, node, kFwInputActivationStateTensor);
  TF_LITE_ENSURE(context, fw_activation_state != nullptr);
  TfLiteTensor* fw_cell_state =
      GetVariableInput(context, node, kFwInputCellStateTensor);
  TF_LITE_ENSURE(context, fw_cell_state != nullptr);

  // Check the shape of input state tensors.
  // These tensor may be 1D or 2D. It's fine as long as the total size is
  // correct.
  TF_LITE_ENSURE_EQ(context, NumElements(fw_activation_state),
                    n_batch * n_fw_output);
  TF_LITE_ENSURE_EQ(context, NumElements(fw_cell_state), n_batch * n_fw_cell);

  // Resize the output tensors.
  TfLiteIntArray* fw_output_size = TfLiteIntArrayCreate(3);
  fw_output_size->data[0] = time_major ? max_time : n_batch;
  fw_output_size->data[1] = time_major ? n_batch : max_time;
  fw_output_size->data[2] =
      params->merge_outputs ? n_bw_output + n_fw_output : n_fw_output;
  TF_LITE_ENSURE_OK(context,
                    context->ResizeTensor(context, fw_output, fw_output_size));

  // The weights are of consistent type, so it suffices to check one.
  const bool is_hybrid_op = IsHybridOp(input, fw_input_to_output_weights);

  TfLiteIntArrayFree(node->temporaries);
  if (is_hybrid_op) {
    node->temporaries = TfLiteIntArrayCreate(
        has_aux_input ? kNumTemporaryTensors : kNumTemporaryTensors - 1);
  } else {
    node->temporaries = TfLiteIntArrayCreate(2);  // the two scratch buffers.
  }
  // Create a scratch buffer tensor.
  node->temporaries->data[kFwScratchBuffer] =
      op_data->scratch_tensor_index + kFwScratchBuffer;
  TfLiteTensor* fw_scratch_buffer =
      GetTemporary(context, node, kFwScratchBuffer);
  fw_scratch_buffer->type = input->type;
  fw_scratch_buffer->allocation_type = kTfLiteArenaRw;

  const TfLiteTensor* fw_input_to_input_weights =
      GetOptionalInputTensor(context, node, kFwInputToInputWeightsTensor);
  const bool fw_use_cifg = (fw_input_to_input_weights == nullptr);
  if (has_aux_input && !fw_use_cifg) {
    TF_LITE_ENSURE_EQ(context, fw_aux_input_to_input_weights->dims->data[0],
                      fw_input_to_input_weights->dims->data[0]);
  }
  TfLiteIntArray* fw_scratch_buffer_size = TfLiteIntArrayCreate(2);
  fw_scratch_buffer_size->data[0] = n_batch;
  if (fw_use_cifg) {
    // Reserving space for Cell, Forget, Output gates
    fw_scratch_buffer_size->data[1] = n_fw_cell * 3;
  } else {
    // Reserving space for Input, Cell, Forget, Output gates
    fw_scratch_buffer_size->data[1] = n_fw_cell * 4;
  }
  TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, fw_scratch_buffer,
                                                   fw_scratch_buffer_size));
  // Same for the backward cell.

  // Check that input tensor dimensions matches with each other.
  TF_LITE_ENSURE_OK(
      context, CheckInputTensorDimensions(context, node, n_input, n_bw_output,
                                          n_bw_cell));

  // Get the pointer to activation_state and cell_state buffer tensors.
  TfLiteTensor* bw_activation_state =
      GetVariableInput(context, node, kBwInputActivationStateTensor);
  TF_LITE_ENSURE(context, bw_activation_state != nullptr);
  TfLiteTensor* bw_cell_state =
      GetVariableInput(context, node, kBwInputCellStateTensor);
  TF_LITE_ENSURE(context, bw_cell_state != nullptr);

  // Resize the output tensors.
  if (!params->merge_outputs) {
    TfLiteTensor* bw_output = GetOutput(context, node, kBwOutputTensor);
    TfLiteIntArray* bw_output_size = TfLiteIntArrayCreate(3);
    bw_output_size->data[0] = time_major ? max_time : n_batch;
    bw_output_size->data[1] = time_major ? n_batch : max_time;
    bw_output_size->data[2] = n_bw_output;
    TF_LITE_ENSURE_OK(
        context, context->ResizeTensor(context, bw_output, bw_output_size));
  }

  // Check the shape of input state tensors.
  // These tensor may be 1D or 2D. It's fine as long as the total size is
  // correct.
  TF_LITE_ENSURE_EQ(context, NumElements(bw_activation_state),
                    n_batch * n_bw_output);
  TF_LITE_ENSURE_EQ(context, NumElements(bw_cell_state), n_batch * n_bw_cell);

  // Create a scratch buffer tensor.
  node->temporaries->data[kBwScratchBuffer] =
      op_data->scratch_tensor_index + kBwScratchBuffer;
  TfLiteTensor* bw_scratch_buffer =
      GetTemporary(context, node, kBwScratchBuffer);
  bw_scratch_buffer->type = input->type;
  bw_scratch_buffer->allocation_type = kTfLiteArenaRw;

  const TfLiteTensor* bw_input_to_input_weights =
      GetOptionalInputTensor(context, node, kBwInputToInputWeightsTensor);
  const bool bw_use_cifg = (bw_input_to_input_weights == nullptr);
  if (has_aux_input && !bw_use_cifg) {
    TF_LITE_ENSURE_EQ(context, bw_aux_input_to_input_weights->dims->data[0],
                      bw_input_to_input_weights->dims->data[0]);
  }
  TfLiteIntArray* bw_scratch_buffer_size = TfLiteIntArrayCreate(2);
  bw_scratch_buffer_size->data[0] = n_batch;
  if (bw_use_cifg) {
    // Reserving space for Cell, Forget, Output gates
    bw_scratch_buffer_size->data[1] = n_bw_cell * 3;
  } else {
    // Reserving space for Input, Cell, Forget, Output gates
    bw_scratch_buffer_size->data[1] = n_bw_cell * 4;
  }
  TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, bw_scratch_buffer,
                                                   bw_scratch_buffer_size));
  if (is_hybrid_op) {
    // Compute the row sums for cached zero_point offset calculation.
    op_data->compute_fw_row_sums = true;
    op_data->compute_bw_row_sums = true;
    // Allocate temporary tensors to store quantized values of input, aux_input
    // (if present), activation_state and cell_state tensors.
    node->temporaries->data[kInputQuantized] =
        op_data->scratch_tensor_index + kInputQuantized;
    TfLiteTensor* input_quantized =
        GetTemporary(context, node, kInputQuantized);
    input_quantized->type = fw_input_to_output_weights->type;
    input_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(input_quantized->dims, input->dims)) {
      TfLiteIntArray* input_quantized_size = TfLiteIntArrayCopy(input->dims);
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, input_quantized,
                                                       input_quantized_size));
    }

    node->temporaries->data[kFwActivationStateQuantized] =
        op_data->scratch_tensor_index + kFwActivationStateQuantized;
    TfLiteTensor* fw_activation_state_quantized =
        GetTemporary(context, node, kFwActivationStateQuantized);
    fw_activation_state_quantized->type = fw_input_to_output_weights->type;
    fw_activation_state_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(fw_activation_state_quantized->dims,
                             fw_activation_state->dims)) {
      TfLiteIntArray* fw_activation_state_quantized_size =
          TfLiteIntArrayCopy(fw_activation_state->dims);
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, fw_activation_state_quantized,
                                         fw_activation_state_quantized_size));
    }
    node->temporaries->data[kBwActivationStateQuantized] =
        op_data->scratch_tensor_index + kBwActivationStateQuantized;
    TfLiteTensor* bw_activation_state_quantized =
        GetTemporary(context, node, kBwActivationStateQuantized);
    bw_activation_state_quantized->type = fw_input_to_output_weights->type;
    bw_activation_state_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(bw_activation_state_quantized->dims,
                             bw_activation_state->dims)) {
      TfLiteIntArray* bw_activation_state_quantized_size =
          TfLiteIntArrayCopy(bw_activation_state->dims);
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, bw_activation_state_quantized,
                                         bw_activation_state_quantized_size));
    }
    node->temporaries->data[kFwCellStateQuantized] =
        op_data->scratch_tensor_index + kFwCellStateQuantized;
    TfLiteTensor* fw_cell_state_quantized =
        GetTemporary(context, node, kFwCellStateQuantized);
    fw_cell_state_quantized->type = fw_input_to_output_weights->type;
    fw_cell_state_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(fw_cell_state_quantized->dims,
                             fw_cell_state->dims)) {
      TfLiteIntArray* fw_cell_state_quantized_size =
          TfLiteIntArrayCopy(fw_cell_state->dims);
      TF_LITE_ENSURE_OK(context,
                        context->ResizeTensor(context, fw_cell_state_quantized,
                                              fw_cell_state_quantized_size));
    }
    node->temporaries->data[kBwCellStateQuantized] =
        op_data->scratch_tensor_index + kBwCellStateQuantized;
    TfLiteTensor* bw_cell_state_quantized =
        GetTemporary(context, node, kBwCellStateQuantized);
    bw_cell_state_quantized->type = fw_input_to_output_weights->type;
    bw_cell_state_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(bw_cell_state_quantized->dims,
                             bw_cell_state->dims)) {
      TfLiteIntArray* bw_cell_state_quantized_size =
          TfLiteIntArrayCopy(bw_cell_state->dims);
      TF_LITE_ENSURE_OK(context,
                        context->ResizeTensor(context, bw_cell_state_quantized,
                                              bw_cell_state_quantized_size));
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
    node->temporaries->data[kAuxInputScalingFactors] =
        op_data->scratch_tensor_index + kAuxInputScalingFactors;
    TfLiteTensor* aux_input_sf =
        GetTemporary(context, node, kAuxInputScalingFactors);
    aux_input_sf->type = kTfLiteFloat32;
    aux_input_sf->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqualsArray(aux_input_sf->dims, 1, scaling_dims)) {
      TfLiteIntArray* aux_input_sf_size = TfLiteIntArrayCreate(1);
      aux_input_sf_size->data[0] = n_batch;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, aux_input_sf,
                                                       aux_input_sf_size));
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
    int recovered_cell_dims[1] = {n_fw_cell};
    if (!TfLiteIntArrayEqualsArray(recovered_cell_weights->dims, 1,
                                   recovered_cell_dims)) {
      TfLiteIntArray* recovered_cell_weights_size = TfLiteIntArrayCreate(1);
      recovered_cell_weights_size->data[0] = n_fw_cell;
      TF_LITE_ENSURE_OK(context,
                        context->ResizeTensor(context, recovered_cell_weights,
                                              recovered_cell_weights_size));
    }

    // Allocate a temporary tensor to store the accumulated int32 values.
    node->temporaries->data[kAccumScratchBuffer] =
        op_data->scratch_tensor_index + kAccumScratchBuffer;
    TfLiteTensor* accum_scratch =
        GetTemporary(context, node, kAccumScratchBuffer);
    accum_scratch->type = kTfLiteInt32;
    accum_scratch->allocation_type = kTfLiteArenaRw;
    int n_cell = std::max(n_fw_cell, n_bw_cell);
    if (has_aux_input) {
      n_cell = std::max(n_cell, fw_aux_input_to_output_weights->dims->data[0]);
      n_cell = std::max(n_cell, bw_aux_input_to_output_weights->dims->data[0]);
    }
    int accum_scratch_dims[2] = {n_cell, n_batch};
    if (!TfLiteIntArrayEqualsArray(accum_scratch->dims, 2,
                                   accum_scratch_dims)) {
      TfLiteIntArray* accum_size = TfLiteIntArrayCreate(2);
      accum_size->data[0] = n_cell;
      accum_size->data[1] = n_batch;
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, accum_scratch, accum_size));
    }

    // Allocate temporary tensors for storing zero-points.
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
    node->temporaries->data[kAuxInputZeroPoints] =
        op_data->scratch_tensor_index + kAuxInputZeroPoints;
    TfLiteTensor* aux_input_zp =
        GetTemporary(context, node, kAuxInputZeroPoints);
    aux_input_zp->type = kTfLiteFloat32;
    aux_input_zp->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqualsArray(aux_input_zp->dims, 1, scaling_dims)) {
      TfLiteIntArray* aux_input_zp_size = TfLiteIntArrayCreate(1);
      aux_input_zp_size->data[0] = n_batch;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, aux_input_zp,
                                                       aux_input_zp_size));
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

    // Allocate temporary tensors for caching row sums for hybrid zero-point
    // calculations.
    int fw_row_sums_rows = fw_use_cifg ? 6 : 8;
    if (has_aux_input) {
      fw_row_sums_rows += fw_use_cifg ? 3 : 4;
    }
    const TfLiteTensor* fw_projection_weights =
        GetOptionalInputTensor(context, node, kFwProjectionWeightsTensor);
    if (fw_projection_weights != nullptr) {
      fw_row_sums_rows += ceil(static_cast<float>(n_fw_output) / n_fw_cell);
    }
    node->temporaries->data[kFwRowSums] =
        op_data->scratch_tensor_index + kFwRowSums;
    TfLiteTensor* fw_row_sums = GetTemporary(context, node, kFwRowSums);
    fw_row_sums->type = kTfLiteInt32;
    fw_row_sums->allocation_type = kTfLiteArenaRwPersistent;
    int fw_row_sums_dims[2] = {fw_row_sums_rows, n_fw_cell};
    if (!TfLiteIntArrayEqualsArray(fw_row_sums->dims, 2, fw_row_sums_dims)) {
      TfLiteIntArray* fw_hybrid_scratch_size = TfLiteIntArrayCreate(2);
      fw_hybrid_scratch_size->data[0] = fw_row_sums_dims[0];
      fw_hybrid_scratch_size->data[1] = fw_row_sums_dims[1];
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, fw_row_sums,
                                                       fw_hybrid_scratch_size));
    }

    int bw_row_sums_rows = bw_use_cifg ? 6 : 8;
    if (has_aux_input) {
      bw_row_sums_rows += bw_use_cifg ? 3 : 4;
    }
    const TfLiteTensor* bw_projection_weights =
        GetOptionalInputTensor(context, node, kBwProjectionWeightsTensor);
    if (bw_projection_weights != nullptr) {
      bw_row_sums_rows += ceil(static_cast<float>(n_bw_output) / n_bw_cell);
    }
    node->temporaries->data[kBwRowSums] =
        op_data->scratch_tensor_index + kBwRowSums;
    TfLiteTensor* bw_row_sums = GetTemporary(context, node, kBwRowSums);
    bw_row_sums->type = kTfLiteInt32;
    bw_row_sums->allocation_type = kTfLiteArenaRwPersistent;
    int bw_row_sums_dims[2] = {bw_row_sums_rows, n_bw_cell};
    if (!TfLiteIntArrayEqualsArray(bw_row_sums->dims, 2, bw_row_sums_dims)) {
      TfLiteIntArray* bw_row_sums_size = TfLiteIntArrayCreate(2);
      bw_row_sums_size->data[0] = bw_row_sums_dims[0];
      bw_row_sums_size->data[1] = bw_row_sums_dims[1];
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, bw_row_sums,
                                                       bw_row_sums_size));
    }

    // Only allocate a temporary tensor for quantized auxiliary input if we are
    // actually going to use it.
    if (has_aux_input) {
      node->temporaries->data[kAuxInputQuantized] =
          op_data->scratch_tensor_index + kAuxInputQuantized;
      TfLiteTensor* aux_input_quantized =
          GetTemporary(context, node, kAuxInputQuantized);
      aux_input_quantized->type = fw_input_to_output_weights->type;
      aux_input_quantized->allocation_type = kTfLiteArenaRw;
      if (!TfLiteIntArrayEqual(aux_input_quantized->dims, aux_input->dims)) {
        TfLiteIntArray* aux_input_quantized_size =
            TfLiteIntArrayCopy(aux_input->dims);
        TF_LITE_ENSURE_OK(context,
                          context->ResizeTensor(context, aux_input_quantized,
                                                aux_input_quantized_size));
      }
    }
  }
  return kTfLiteOk;
}