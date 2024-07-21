TfLiteStatus PrepareImpl(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteFullyConnectedParams*>(node->builtin_data);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);
  // Check we have all the inputs and outputs we need.
  TF_LITE_ENSURE(context, node->inputs->size == 2 || node->inputs->size == 3);
  // Shuffled formats need a workspace to store the shuffled input activations.
  const int expected_outputs_count =
      params->weights_format == kTfLiteFullyConnectedWeightsFormatDefault ? 1
                                                                          : 2;
  TF_LITE_ENSURE_EQ(context, node->outputs->size, expected_outputs_count);

  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensor, &input));
  const TfLiteTensor* filter;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kWeightsTensor, &filter));
  const TfLiteTensor* bias =
      (node->inputs->size == 3)
          ? GetOptionalInputTensor(context, node, kBiasTensor)
          : nullptr;
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  // Check proper datatype match among all Input Tensors
  TF_LITE_ENSURE_STATUS(
      CheckTypes(context, input, filter, bias, output, params));

  // Check all the parameters of tensor match within themselves and match the
  // input configuration.
  int input_size = 1;
  for (int i = 0; i < input->dims->size; i++) {
    input_size *= input->dims->data[i];
  }

  TF_LITE_ENSURE_EQ(context, NumDimensions(filter), 2);
  const int batch_size = input_size / filter->dims->data[1];
  const int num_units = filter->dims->data[0];

  if (bias) {
    TF_LITE_ENSURE_EQ(context, NumElements(bias), SizeOfDimension(filter, 0));
  }

  // Note that quantized inference requires that all tensors have their
  // parameters set. This is usually done during quantized training.
  if (input->type == kTfLiteUInt8 || input->type == kTfLiteInt8 ||
      input->type == kTfLiteInt16) {
    double real_multiplier = 0.0;
    TF_LITE_ENSURE_STATUS(GetQuantizedConvolutionMultipler(
        context, input, filter, bias, output, &real_multiplier));
    int exponent;
    QuantizeMultiplier(real_multiplier, &data->output_multiplier, &exponent);
    data->output_shift = exponent;
    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
        context, params->activation, output, &data->output_activation_min,
        &data->output_activation_max));
  }

  if (input->type == kTfLiteInt16 && output->type == kTfLiteInt16) {
    TF_LITE_ENSURE_EQ(context, input->params.zero_point, 0);
    TF_LITE_ENSURE_EQ(context, output->params.zero_point, 0);
  }

  // If we have to perform on-the-fly quantization (with quantized weights and
  // float inputs) first we need to quantize the inputs. Allocate a temporary
  // buffer to store the intermediate quantized values.
  // Additionally, we allocate a temporary buffer to store the accumulated
  // quantized values prior to multiplication by the scaling factor.
  const bool is_hybrid =
      (input->type == kTfLiteFloat32 &&
       (filter->type == kTfLiteUInt8 || filter->type == kTfLiteInt8));
  const bool is_sparse = filter->sparsity != nullptr;
  if (is_hybrid) {
    TfLiteIntArrayFree(node->temporaries);
    data->compute_row_sums = true;
    if (is_sparse) {
      node->temporaries = TfLiteIntArrayCreate(6);
    } else {
      node->temporaries = TfLiteIntArrayCreate(5);
    }
    node->temporaries->data[0] = data->scratch_tensor_index;

    TfLiteTensor* input_quantized;
    TF_LITE_ENSURE_OK(context, GetTemporarySafe(context, node, /*index=*/0,
                                                &input_quantized));
    input_quantized->type = filter->type;
    input_quantized->allocation_type = kTfLiteArenaRw;

    TfLiteIntArray* input_quantized_size = TfLiteIntArrayCopy(input->dims);
    TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, input_quantized,
                                                     input_quantized_size));

    node->temporaries->data[1] = data->scratch_tensor_index + 1;
    TfLiteTensor* scaling_factors;
    TF_LITE_ENSURE_OK(context, GetTemporarySafe(context, node, /*index=*/1,
                                                &scaling_factors));
    scaling_factors->type = kTfLiteFloat32;
    scaling_factors->allocation_type = kTfLiteArenaRw;

    int scaling_dims[1] = {batch_size};
    if (!TfLiteIntArrayEqualsArray(scaling_factors->dims, 1, scaling_dims)) {
      TfLiteIntArray* scaling_factors_size = TfLiteIntArrayCreate(1);
      scaling_factors_size->data[0] = batch_size;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, scaling_factors,
                                                       scaling_factors_size));
    }

    node->temporaries->data[2] = data->scratch_tensor_index + 2;
    TfLiteTensor* accum_scratch;
    TF_LITE_ENSURE_OK(
        context, GetTemporarySafe(context, node, /*index=*/2, &accum_scratch));
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

    node->temporaries->data[3] = data->scratch_tensor_index + 3;
    TfLiteTensor* input_offsets;
    TF_LITE_ENSURE_OK(
        context, GetTemporarySafe(context, node, /*index=*/3, &input_offsets));
    input_offsets->type = kTfLiteInt32;
    input_offsets->allocation_type = kTfLiteArenaRw;
    if (!TfLiteIntArrayEqualsArray(input_offsets->dims, 1, scaling_dims)) {
      TfLiteIntArray* input_offsets_size = TfLiteIntArrayCreate(1);
      input_offsets_size->data[0] = batch_size;
      TF_LITE_ENSURE_OK(context, context->ResizeTensor(context, input_offsets,
                                                       input_offsets_size));
    }
    node->temporaries->data[4] = data->scratch_tensor_index + 4;
    TfLiteTensor* row_sums;
    TF_LITE_ENSURE_OK(context,
                      GetTemporarySafe(context, node, /*index=*/4, &row_sums));
    row_sums->type = kTfLiteInt32;
    row_sums->allocation_type = kTfLiteArenaRwPersistent;
    int row_sums_dims[1] = {num_units};
    if (!TfLiteIntArrayEqualsArray(row_sums->dims, 1, row_sums_dims)) {
      TfLiteIntArray* row_sums_size = TfLiteIntArrayCreate(1);
      row_sums_size->data[0] = row_sums_dims[0];
      TF_LITE_ENSURE_OK(
          context, context->ResizeTensor(context, row_sums, row_sums_size));
    }

    if (is_sparse) {
      data->ledger_initialized = false;
      node->temporaries->data[5] = data->scratch_tensor_index + 5;
      TfLiteTensor* filter_ledger =
          &context->tensors[node->temporaries->data[5]];
      auto status =
          CreateLedgerTensor(filter->sparsity, context, filter_ledger);
      if (status != kTfLiteOk) return status;
    }
  }

  // Resize output.
  TfLiteIntArray* output_size_array = nullptr;
  if (params->keep_num_dims) {
    // When number of dimensions are kept the filter operates along the last
    // dimensions. In other words, for an input tensor with shape
    // [batch_size, ..., n_inputs] and a filter of shape [n_inputs, n_units]
    // this Op produces an output of shape [batch_size, ..., n_units].
    TF_LITE_ENSURE_EQ(context, input->dims->data[input->dims->size - 1],
                      SizeOfDimension(filter, 1));
    output_size_array = TfLiteIntArrayCopy(input->dims);
    output_size_array->data[output_size_array->size - 1] = num_units;
  } else {
    // Otherwise, the output is (potentially flattened to) a 2-D matrix.
    output_size_array = TfLiteIntArrayCreate(2);
    output_size_array->data[0] = batch_size;
    output_size_array->data[1] = num_units;
  }
  TF_LITE_ENSURE_OK(context,
                    context->ResizeTensor(context, output, output_size_array));

  return kTfLiteOk;
}