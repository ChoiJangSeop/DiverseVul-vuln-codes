TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  // Check we have all the inputs and outputs we need.
  TF_LITE_ENSURE_EQ(context, node->inputs->size, 5);
  TF_LITE_ENSURE_EQ(context, node->outputs->size, 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* input_weights = GetInput(context, node, kWeightsTensor);
  const TfLiteTensor* recurrent_weights =
      GetInput(context, node, kRecurrentWeightsTensor);
  const TfLiteTensor* bias = GetInput(context, node, kBiasTensor);
  const TfLiteTensor* hidden_state =
      GetInput(context, node, kHiddenStateTensor);

  // Check all the parameters of tensor match within themselves and match the
  // input configuration.
  const int batch_size = input->dims->data[0];
  const int num_units = input_weights->dims->data[0];
  TF_LITE_ENSURE_EQ(context, input->dims->data[1],
                    input_weights->dims->data[1]);
  TF_LITE_ENSURE_EQ(context, input_weights->dims->data[0], bias->dims->data[0]);
  TF_LITE_ENSURE_EQ(context, recurrent_weights->dims->data[0],
                    bias->dims->data[0]);
  TF_LITE_ENSURE_EQ(context, recurrent_weights->dims->data[1],
                    bias->dims->data[0]);
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, kTfLiteFloat32);
  TF_LITE_ENSURE_TYPES_EQ(context, input_weights->type,
                          recurrent_weights->type);
  TF_LITE_ENSURE_EQ(context, NumDimensions(hidden_state), 2);
  TF_LITE_ENSURE_EQ(context, hidden_state->dims->data[0], batch_size);
  TF_LITE_ENSURE_EQ(context, hidden_state->dims->data[1], num_units);

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  // Resize output.
  TfLiteIntArray* output_size_array = TfLiteIntArrayCreate(2);
  output_size_array->data[0] = batch_size;
  output_size_array->data[1] = num_units;
  TF_LITE_ENSURE_OK(context,
                    context->ResizeTensor(context, output, output_size_array));

  const bool is_hybrid = IsHybridOp(input, input_weights);

  // Allocate temporary tensors to store quantized values of input and
  // hidden_state tensors.
  if (is_hybrid) {
    auto* op_data = reinterpret_cast<OpData*>(node->user_data);
    op_data->compute_row_sums = true;
    TfLiteIntArrayFree(node->temporaries);
    node->temporaries = TfLiteIntArrayCreate(6);
    node->temporaries->data[0] = op_data->scratch_tensor_index;
    TfLiteTensor* input_quantized = GetTemporary(context, node, /*index=*/0);
    input_quantized->type = input_weights->type;
    input_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(input_quantized->dims, input->dims)) {
      TfLiteIntArray* input_quantized_size = TfLiteIntArrayCopy(input->dims);
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, input_quantized,
                                                       input_quantized_size));
    }
    node->temporaries->data[1] = op_data->scratch_tensor_index + 1;
    TfLiteTensor* hidden_state_quantized =
        GetTemporary(context, node, /*index=*/1);
    hidden_state_quantized->type = input_weights->type;
    hidden_state_quantized->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqual(hidden_state_quantized->dims,
                             hidden_state->dims)) {
      TfLiteIntArray* hidden_state_quantized_size =
          TfLiteIntArrayCopy(hidden_state->dims);
      TF_LITE_ENSURE_OK(context,
                        context->ResizeTensor(context, hidden_state_quantized,
                                              hidden_state_quantized_size));
    }
    node->temporaries->data[2] = op_data->scratch_tensor_index + 2;
    TfLiteTensor* scaling_factors = GetTemporary(context, node, /*index=*/2);
    scaling_factors->type = kTfLiteFloat32;
    scaling_factors->allocation_type = kTfLiteArenaRw;
    int scaling_dims[1] = {batch_size};
    if (!TfLiteIntArrayEqualsArray(scaling_factors->dims, 1, scaling_dims)) {
      TfLiteIntArray* scaling_factors_size = TfLiteIntArrayCreate(1);
      scaling_factors_size->data[0] = batch_size;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, scaling_factors,
                                                       scaling_factors_size));
    }
    node->temporaries->data[3] = op_data->scratch_tensor_index + 3;
    TfLiteTensor* accum_scratch = GetTemporary(context, node, /*index=*/3);
    accum_scratch->type = kTfLiteInt32;
    accum_scratch->allocation_type = kTfLiteArenaRw;
    int accum_scratch_dims[2] = {num_units, batch_size};
    if (!TfLiteIntArrayEqualsArray(accum_scratch->dims, 2,
                                   accum_scratch_dims)) {
      TfLiteIntArray* accum_scratch_size = TfLiteIntArrayCreate(2);
      accum_scratch_size->data[0] = accum_scratch_dims[0];
      accum_scratch_size->data[1] = accum_scratch_dims[1];
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, accum_scratch,
                                                       accum_scratch_size));
    }
    node->temporaries->data[4] = op_data->scratch_tensor_index + 4;
    TfLiteTensor* zero_points = GetTemporary(context, node, /*index=*/4);
    zero_points->type = kTfLiteInt32;
    zero_points->allocation_type = kTfLiteArenaRw;
    int zero_points_dims[1] = {batch_size};
    if (!TfLiteIntArrayEqualsArray(zero_points->dims, 1, zero_points_dims)) {
      TfLiteIntArray* zero_points_size = TfLiteIntArrayCreate(1);
      zero_points_size->data[0] = batch_size;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, zero_points,
                                                       zero_points_size));
    }
    node->temporaries->data[5] = op_data->scratch_tensor_index + 5;
    TfLiteTensor* row_sums = GetTemporary(context, node, /*index=*/5);
    row_sums->type = kTfLiteInt32;
    row_sums->allocation_type = kTfLiteArenaRwPersistent;
    int row_sums_dims[2] = {2, num_units};
    if (!TfLiteIntArrayEqualsArray(row_sums->dims, 2, row_sums_dims)) {
      TfLiteIntArray* row_sums_size = TfLiteIntArrayCreate(2);
      row_sums_size->data[0] = row_sums_dims[0];
      row_sums_size->data[1] = row_sums_dims[1];
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, row_sums, row_sums_size));
    }
  }
  return kTfLiteOk;
}