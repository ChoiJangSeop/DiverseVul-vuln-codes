TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  const auto* params = reinterpret_cast<TfLiteBidirectionalSequenceRNNParams*>(
      node->builtin_data);

  // Check we have all the inputs and outputs we need.
  TF_LITE_ENSURE_EQ(context, node->inputs->size, 12);
  TF_LITE_ENSURE_EQ(context, node->outputs->size,
                    params->merge_outputs ? 1 : 2);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* fw_input_weights =
      GetInput(context, node, kFwWeightsTensor);
  const TfLiteTensor* fw_recurrent_weights =
      GetInput(context, node, kFwRecurrentWeightsTensor);
  const TfLiteTensor* fw_bias = GetInput(context, node, kFwBiasTensor);
  const TfLiteTensor* fw_hidden_state =
      GetInput(context, node, kFwHiddenStateTensor);
  const TfLiteTensor* bw_input_weights =
      GetInput(context, node, kBwWeightsTensor);
  const TfLiteTensor* bw_recurrent_weights =
      GetInput(context, node, kBwRecurrentWeightsTensor);
  const TfLiteTensor* bw_bias = GetInput(context, node, kBwBiasTensor);
  const TfLiteTensor* bw_hidden_state =
      GetInput(context, node, kBwHiddenStateTensor);

  const TfLiteTensor* aux_input =
      GetOptionalInputTensor(context, node, kAuxInputTensor);
  const TfLiteTensor* fw_aux_input_weights =
      GetOptionalInputTensor(context, node, kFwAuxWeightsTensor);
  const TfLiteTensor* bw_aux_input_weights =
      GetOptionalInputTensor(context, node, kBwAuxWeightsTensor);

  const bool aux_inputs_weights_or_none =
      ((fw_aux_input_weights != nullptr) &&
       (bw_aux_input_weights != nullptr)) ||
      ((fw_aux_input_weights == nullptr) && (bw_aux_input_weights == nullptr));
  TF_LITE_ENSURE(context, aux_inputs_weights_or_none);
  const bool has_aux_input = (fw_aux_input_weights != nullptr);

  // Check all the parameters of tensor match within themselves and match the
  // input configuration.
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, kTfLiteFloat32);

  TF_LITE_ENSURE_EQ(context, input->dims->size, 3);
  const bool time_major = params->time_major;
  const int batch_size =
      (time_major) ? input->dims->data[1] : input->dims->data[0];
  const int max_time =
      (time_major) ? input->dims->data[0] : input->dims->data[1];
  const int fw_num_units = fw_input_weights->dims->data[0];
  const int bw_num_units = bw_input_weights->dims->data[0];
  TF_LITE_ENSURE_EQ(context, input->dims->data[2],
                    fw_input_weights->dims->data[1]);
  TF_LITE_ENSURE_EQ(context, input->dims->data[2],
                    bw_input_weights->dims->data[1]);
  TF_LITE_ENSURE_EQ(context, fw_input_weights->dims->data[0],
                    fw_bias->dims->data[0]);
  TF_LITE_ENSURE_EQ(context, bw_input_weights->dims->data[0],
                    bw_bias->dims->data[0]);
  TF_LITE_ENSURE_EQ(context, fw_recurrent_weights->dims->data[0],
                    fw_bias->dims->data[0]);
  TF_LITE_ENSURE_EQ(context, bw_recurrent_weights->dims->data[1],
                    bw_bias->dims->data[0]);
  TF_LITE_ENSURE_EQ(context, NumDimensions(fw_hidden_state), 2);
  TF_LITE_ENSURE_EQ(context, fw_hidden_state->dims->data[0], batch_size);
  TF_LITE_ENSURE_EQ(context, fw_hidden_state->dims->data[1], fw_num_units);
  TF_LITE_ENSURE_EQ(context, NumDimensions(bw_hidden_state), 2);
  TF_LITE_ENSURE_EQ(context, bw_hidden_state->dims->data[0], batch_size);
  TF_LITE_ENSURE_EQ(context, bw_hidden_state->dims->data[1], bw_num_units);

  if (has_aux_input) {
    // Check that aux_input has the same dimensions (except last) as the input.
    TF_LITE_ASSERT_EQ(aux_input->dims->data[0], input->dims->data[0]);
    TF_LITE_ASSERT_EQ(aux_input->dims->data[1], input->dims->data[1]);
    // Check that aux_input_weights has the same dimensions (except last) as
    // the input_weights.
    TF_LITE_ASSERT_EQ(fw_aux_input_weights->dims->data[0], fw_num_units);
    TF_LITE_ASSERT_EQ(bw_aux_input_weights->dims->data[0], bw_num_units);
    TF_LITE_ASSERT_EQ(aux_input->dims->data[2],
                      fw_aux_input_weights->dims->data[1]);
    TF_LITE_ASSERT_EQ(aux_input->dims->data[2],
                      bw_aux_input_weights->dims->data[1]);
  }

  if (IsHybridOp(input, fw_input_weights)) {
    OpData* op_data = reinterpret_cast<OpData*>(node->user_data);
    op_data->fw_compute_row_sums = true;
    op_data->bw_compute_row_sums = true;
    TfLiteIntArrayFree(node->temporaries);
    if (has_aux_input) {
      node->temporaries = TfLiteIntArrayCreate(kNumTemporaryTensors);
    } else {
      // No need to create a temporary tensor for the non-existent aux_input.
      node->temporaries = TfLiteIntArrayCreate(kNumTemporaryTensors - 1);
    }

    node->temporaries->data[kInputQuantized] =
        op_data->scratch_tensor_index + kInputQuantized;
    TfLiteTensor* input_quantized =
        GetTemporary(context, node, kInputQuantized);
    input_quantized->type = fw_input_weights->type;
    input_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(input_quantized->dims, input->dims)) {
      TfLiteIntArray* input_quantized_size = TfLiteIntArrayCopy(input->dims);
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, input_quantized,
                                                       input_quantized_size));
    }

    node->temporaries->data[kFwHiddenStateQuantized] =
        op_data->scratch_tensor_index + kFwHiddenStateQuantized;
    TfLiteTensor* fw_hidden_state_quantized =
        GetTemporary(context, node, kFwHiddenStateQuantized);
    fw_hidden_state_quantized->type = fw_input_weights->type;
    fw_hidden_state_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(fw_hidden_state_quantized->dims,
                             fw_hidden_state->dims)) {
      TfLiteIntArray* fw_hidden_state_quantized_size =
          TfLiteIntArrayCopy(fw_hidden_state->dims);
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, fw_hidden_state_quantized,
                                         fw_hidden_state_quantized_size));
    }

    node->temporaries->data[kBwHiddenStateQuantized] =
        op_data->scratch_tensor_index + kBwHiddenStateQuantized;
    TfLiteTensor* bw_hidden_state_quantized =
        GetTemporary(context, node, kBwHiddenStateQuantized);
    bw_hidden_state_quantized->type = fw_input_weights->type;
    bw_hidden_state_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(bw_hidden_state_quantized->dims,
                             bw_hidden_state->dims)) {
      TfLiteIntArray* bw_hidden_state_quantized_size =
          TfLiteIntArrayCopy(bw_hidden_state->dims);
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, bw_hidden_state_quantized,
                                         bw_hidden_state_quantized_size));
    }

    // Allocate temporary tensors to store scaling factors of quantization.
    node->temporaries->data[kScalingFactors] =
        op_data->scratch_tensor_index + kScalingFactors;
    TfLiteTensor* scaling_factors =
        GetTemporary(context, node, kScalingFactors);
    scaling_factors->type = kTfLiteFloat32;
    scaling_factors->allocation_type = kTfLiteArenaRw;
    int scaling_dims[1] = {batch_size};
    if (!TfLiteIntArrayEqualsArray(scaling_factors->dims, 1, scaling_dims)) {
      TfLiteIntArray* scaling_factors_size = TfLiteIntArrayCreate(1);
      scaling_factors_size->data[0] = batch_size;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, scaling_factors,
                                                       scaling_factors_size));
    }
    node->temporaries->data[kAccumScratch] =
        op_data->scratch_tensor_index + kAccumScratch;
    TfLiteTensor* accum_scratch = GetTemporary(context, node, kAccumScratch);
    accum_scratch->type = kTfLiteInt32;
    accum_scratch->allocation_type = kTfLiteArenaRw;
    int accum_scratch_dims[2] = {std::max(fw_num_units, bw_num_units),
                                 batch_size};
    if (!TfLiteIntArrayEqualsArray(accum_scratch->dims, 2,
                                   accum_scratch_dims)) {
      TfLiteIntArray* accum_scratch_size = TfLiteIntArrayCreate(2);
      accum_scratch_size->data[0] = accum_scratch_dims[0];
      accum_scratch_size->data[1] = accum_scratch_dims[1];
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, accum_scratch,
                                                       accum_scratch_size));
    }
    node->temporaries->data[kZeroPoints] =
        op_data->scratch_tensor_index + kZeroPoints;
    TfLiteTensor* zero_points =
        GetTemporary(context, node, /*index=*/kZeroPoints);
    zero_points->type = kTfLiteInt32;
    zero_points->allocation_type = kTfLiteArenaRw;
    int zero_points_dims[1] = {batch_size};
    if (!TfLiteIntArrayEqualsArray(zero_points->dims, 1, zero_points_dims)) {
      TfLiteIntArray* zero_points_size = TfLiteIntArrayCreate(1);
      zero_points_size->data[0] = batch_size;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, zero_points,
                                                       zero_points_size));
    }
    const int num_row_sums = has_aux_input ? 3 : 2;
    node->temporaries->data[kFwRowSums] =
        op_data->scratch_tensor_index + kFwRowSums;
    TfLiteTensor* fw_row_sums =
        GetTemporary(context, node, /*index=*/kFwRowSums);
    fw_row_sums->type = kTfLiteInt32;
    fw_row_sums->allocation_type = kTfLiteArenaRwPersistent;
    int fw_row_sums_dims[2] = {num_row_sums, fw_num_units};
    if (!TfLiteIntArrayEqualsArray(fw_row_sums->dims, 2, fw_row_sums_dims)) {
      TfLiteIntArray* fw_row_sums_size = TfLiteIntArrayCreate(2);
      fw_row_sums_size->data[0] = fw_row_sums_dims[0];
      fw_row_sums_size->data[1] = fw_row_sums_dims[1];
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, fw_row_sums,
                                                       fw_row_sums_size));
    }
    node->temporaries->data[kBwRowSums] =
        op_data->scratch_tensor_index + kBwRowSums;
    TfLiteTensor* bw_row_sums = GetTemporary(context, node,
                                             /*index=*/kBwRowSums);
    bw_row_sums->type = kTfLiteInt32;
    bw_row_sums->allocation_type = kTfLiteArenaRwPersistent;
    int bw_row_sums_dims[2] = {num_row_sums, bw_num_units};
    if (!TfLiteIntArrayEqualsArray(bw_row_sums->dims, 2, bw_row_sums_dims)) {
      TfLiteIntArray* bw_row_sums_size = TfLiteIntArrayCreate(2);
      bw_row_sums_size->data[0] = bw_row_sums_dims[0];
      bw_row_sums_size->data[1] = bw_row_sums_dims[1];
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, bw_row_sums,
                                                       bw_row_sums_size));
    }
    if (has_aux_input) {
      node->temporaries->data[kAuxInputQuantized] =
          op_data->scratch_tensor_index + kAuxInputQuantized;
      TfLiteTensor* aux_input_quantized =
          GetTemporary(context, node, kAuxInputQuantized);
      aux_input_quantized->type = fw_input_weights->type;
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

  // Resize outputs.
  TfLiteTensor* fw_output = GetOutput(context, node, kFwOutputTensor);
  TfLiteIntArray* fw_output_size_array = TfLiteIntArrayCreate(3);
  fw_output_size_array->data[0] = (time_major) ? max_time : batch_size;
  fw_output_size_array->data[1] = (time_major) ? batch_size : max_time;
  fw_output_size_array->data[2] =
      params->merge_outputs ? fw_num_units + bw_num_units : fw_num_units;
  TF_LITE_ENSURE_OK(
      context, context->ResizeTensor(context, fw_output, fw_output_size_array));
  if (!params->merge_outputs) {
    TfLiteTensor* bw_output = GetOutput(context, node, kBwOutputTensor);
    TfLiteIntArray* bw_output_size_array = TfLiteIntArrayCreate(3);
    bw_output_size_array->data[0] = batch_size;
    bw_output_size_array->data[1] = max_time;
    bw_output_size_array->data[2] = bw_num_units;
    TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, bw_output,
                                                     bw_output_size_array));
  }

  return kTfLiteOk;
}