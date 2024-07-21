TfLiteStatus InitializeTemporaries(TfLiteContext* context, TfLiteNode* node,
                                   OpContext* op_context) {
  // Create temporary tensors to hold transposed LHS/RHS.
  OpData* op_data = reinterpret_cast<OpData*>(node->user_data);
  const TfLiteTensor* lhs = op_context->lhs;
  const TfLiteTensor* rhs = op_context->rhs;
  TfLiteIntArrayFree(node->temporaries);
  // For "hybrid" quantization, we impose the constraint that the LHS
  // is float (typically an activation from a prior layer) and the RHS
  // is quantized int8.
  bool is_hybrid =
      (op_context->lhs->type == kTfLiteFloat32 && rhs->type == kTfLiteInt8);
  if (is_hybrid) {
    node->temporaries = TfLiteIntArrayCreate(kNumTempTensorsForAdjoints +
                                             kNumTempTensorsForHybrid);
  } else {
    node->temporaries = TfLiteIntArrayCreate(kNumTempTensorsForAdjoints);
  }

  const int lhs_rank = NumDimensions(lhs);
  const int rhs_rank = NumDimensions(rhs);
  const int batch_size = op_context->params->adj_x
                             ? lhs->dims->data[lhs_rank - 2]
                             : lhs->dims->data[lhs_rank - 1];
  const int num_units = op_context->params->adj_x
                            ? lhs->dims->data[lhs_rank - 1]
                            : lhs->dims->data[lhs_rank - 2];

  // Temp tensor for Transposed LHS;
  {
    node->temporaries->data[0] = op_data->scratch_tensor_index;
    TfLiteTensor* scratch_buffer = GetTemporary(context, node, /*index=*/0);
    TfLiteIntArray* scratch_buffer_size = TfLiteIntArrayCreate(lhs_rank);
    for (int i = 0; i < lhs_rank - 2; ++i) {
      scratch_buffer_size->data[i] = lhs->dims->data[i];
    }
    // Swap last two dimensions.
    scratch_buffer_size->data[lhs_rank - 2] = lhs->dims->data[lhs_rank - 1];
    scratch_buffer_size->data[lhs_rank - 1] = lhs->dims->data[lhs_rank - 2];

    scratch_buffer->type = op_context->lhs->type;
    scratch_buffer->allocation_type = kTfLiteArenaRw;
    TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, scratch_buffer,
                                                     scratch_buffer_size));
  }

  // We need a temp buffer for the RHS if we need to transpose the RHS. We
  // transpose by default, so that the two inputs (LHS and RHS) are in a proper
  // layout for our fast matrix multiplication routines. If the transpose flag
  // is set by the caller, the data is already in the desired layout.
  {
    node->temporaries->data[1] = op_data->scratch_tensor_index + 1;
    TfLiteTensor* scratch_buffer = GetTemporary(context, node, /*index=*/1);
    const TfLiteTensor* rhs = op_context->rhs;
    int rhs_rank = NumDimensions(rhs);
    TfLiteIntArray* scratch_buffer_size = TfLiteIntArrayCreate(rhs_rank);
    for (int i = 0; i < rhs_rank - 2; ++i) {
      scratch_buffer_size->data[i] = rhs->dims->data[i];
    }
    // Swap last two dimensions.
    scratch_buffer_size->data[rhs_rank - 2] = rhs->dims->data[rhs_rank - 1];
    scratch_buffer_size->data[rhs_rank - 1] = rhs->dims->data[rhs_rank - 2];

    if (IsConstantTensor(op_context->rhs)) {
      scratch_buffer->allocation_type = kTfLiteArenaRwPersistent;
    } else {
      scratch_buffer->allocation_type = kTfLiteArenaRw;
    }
    scratch_buffer->type = op_context->rhs->type;
    scratch_buffer->allocation_type = kTfLiteArenaRw;
    TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, scratch_buffer,
                                                     scratch_buffer_size));
  }

  // If we have to perform on-the-fly quantization (with quantized weights and
  // float inputs) first we need to quantize the inputs. Allocate temporary
  // buffer to store the intermediate quantized values, the batch scaling
  // factors, the accumulator buffer (optimized version), the input offsets,
  // and the sums of the rows for each weights matrix.
  // RHS = weights, LHS = inputs
  if (is_hybrid) {
    // Calculate the total number of LHS batches.
    int num_batches = 1;
    for (int i = 0; i < lhs_rank - 2; ++i) {
      num_batches *= lhs->dims->data[i];
    }
    int num_weights_matrices = 1;
    for (int i = 0; i < rhs_rank - 2; ++i) {
      num_weights_matrices *= rhs->dims->data[i];
    }
    op_data->compute_row_sums = true;
    node->temporaries->data[2] = op_data->scratch_tensor_index + 2;
    TfLiteTensor* input_quantized = GetTemporary(context, node, /*index=*/2);
    input_quantized->type = op_context->rhs->type;
    input_quantized->allocation_type = kTfLiteArenaRw;

    TfLiteIntArray* input_quantized_size =
        TfLiteIntArrayCopy(op_context->lhs->dims);
    TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, input_quantized,
                                                     input_quantized_size));

    node->temporaries->data[3] = op_data->scratch_tensor_index + 3;
    TfLiteTensor* scaling_factors = GetTemporary(context, node, /*index=*/3);
    scaling_factors->type = kTfLiteFloat32;
    scaling_factors->allocation_type = kTfLiteArenaRw;
    // Total size of scaling factors is batch size * number of total batches
    int scaling_dims[1] = {num_batches * batch_size};
    if (!TfLiteIntArrayEqualsArray(scaling_factors->dims, 1, scaling_dims)) {
      TfLiteIntArray* scaling_factors_size = TfLiteIntArrayCreate(1);
      scaling_factors_size->data[0] = batch_size;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, scaling_factors,
                                                       scaling_factors_size));
    }

    node->temporaries->data[4] = op_data->scratch_tensor_index + 4;
    TfLiteTensor* accum_scratch = GetTemporary(context, node, /*index=*/4);
    accum_scratch->type = kTfLiteInt32;
    accum_scratch->allocation_type = kTfLiteArenaRw;
    int accum_scratch_dims[2] = {num_units, batch_size};
    if (!TfLiteIntArrayEqualsArray(accum_scratch->dims, 2,
                                   accum_scratch_dims)) {
      TfLiteIntArray* accum_size = TfLiteIntArrayCreate(2);
      accum_size->data[0] = num_units;
      accum_size->data[1] = batch_size;
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, accum_scratch, accum_size));
    }

    node->temporaries->data[5] = op_data->scratch_tensor_index + 5;
    TfLiteTensor* input_offsets = GetTemporary(context, node, /*index=*/5);
    input_offsets->type = kTfLiteInt32;
    input_offsets->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqualsArray(input_offsets->dims, 1, scaling_dims)) {
      TfLiteIntArray* input_offsets_size = TfLiteIntArrayCreate(1);
      input_offsets_size->data[0] = num_batches * batch_size;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, input_offsets,
                                                       input_offsets_size));
    }
    node->temporaries->data[6] = op_data->scratch_tensor_index + 6;
    TfLiteTensor* row_sums = GetTemporary(context, node, /*index=*/6);
    row_sums->type = kTfLiteInt32;
    row_sums->allocation_type = kTfLiteArenaRwPersistent;
    int row_sums_dims[1] = {num_weights_matrices * num_units};
    if (!TfLiteIntArrayEqualsArray(row_sums->dims, 1, row_sums_dims)) {
      TfLiteIntArray* row_sums_size = TfLiteIntArrayCreate(1);
      row_sums_size->data[0] = row_sums_dims[0];
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, row_sums, row_sums_size));
    }
  }

  return kTfLiteOk;
}