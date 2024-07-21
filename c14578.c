TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteEmbeddingLookupSparseParams*>(node->builtin_data);
  TfLiteTensor* output = GetOutput(context, node, 0);
  const TfLiteTensor* ids = GetInput(context, node, 0);
  const TfLiteTensor* indices = GetInput(context, node, 1);
  const TfLiteTensor* dense_shape = GetInput(context, node, 2);
  const TfLiteTensor* weights = GetInput(context, node, 3);
  const TfLiteTensor* value = GetInput(context, node, 4);

  const int lookup_rank = SizeOfDimension(indices, 1);
  const int embedding_rank = NumDimensions(value);
  const int num_lookups = SizeOfDimension(ids, 0);
  const int num_rows = SizeOfDimension(value, 0);

  // The last dimension gets replaced by the embedding.
  const int output_rank = (lookup_rank - 1) + (embedding_rank - 1);

  // Make sure that the actual dense shape of the sparse tensor represented by
  // (loopkup, indices, dense_shape) is consistent.
  TF_LITE_ENSURE_EQ(context, SizeOfDimension(dense_shape, 0), lookup_rank);

  // Resize output tensor.
  TfLiteIntArray* output_shape = TfLiteIntArrayCreate(output_rank);
  int k = 0;
  int embedding_size = 1;
  int lookup_size = 1;
  for (int i = 0; i < lookup_rank - 1; i++, k++) {
    const int dim = dense_shape->data.i32[i];
    lookup_size *= dim;
    output_shape->data[k] = dim;
  }
  for (int i = 1; i < embedding_rank; i++, k++) {
    const int dim = SizeOfDimension(value, i);
    embedding_size *= dim;
    output_shape->data[k] = dim;
  }
  TF_LITE_ENSURE_STATUS(context->ResizeTensor(context, output, output_shape));
  const int output_size = lookup_size * embedding_size;
  TfLiteTensorRealloc(output_size * sizeof(float), output);

  float* output_ptr = GetTensorData<float>(output);
  const float* weights_ptr = GetTensorData<float>(weights);
  const float* value_ptr = GetTensorData<float>(value);

  std::fill_n(output_ptr, output_size, 0.0f);

  // Keep track of the current bucket for aggregation/combination.
  int current_output_offset = 0;
  float current_total_weight = 0.0;
  float current_squares_weight = 0.0;
  int num_elements = 0;

  for (int i = 0; i < num_lookups; i++) {
    int idx = ids->data.i32[i];
    if (idx >= num_rows || idx < 0) {
      context->ReportError(context,
                           "Embedding Lookup Sparse: index out of bounds. "
                           "Got %d, and bounds are [0, %d]",
                           idx, num_rows - 1);
      return kTfLiteError;
    }

    // Check where we need to aggregate.
    const int example_indices_offset = i * lookup_rank;
    int output_bucket = 0;
    int stride = 1;
    for (int k = (lookup_rank - 1) - 1; k >= 0; k--) {
      output_bucket += indices->data.i32[example_indices_offset + k] * stride;
      stride *= dense_shape->data.i32[k];
    }
    const int output_offset = output_bucket * embedding_size;

    // If we are in a new aggregation bucket and the combiner is not the sum,
    // go back and finalize the result of the previous bucket.
    if (output_offset != current_output_offset) {
      FinalizeAggregation(params->combiner, num_elements, current_total_weight,
                          current_squares_weight, embedding_size,
                          &output_ptr[current_output_offset]);

      // Track next bucket.
      num_elements = 0;
      current_total_weight = 0.0;
      current_squares_weight = 0.0;
      current_output_offset = output_offset;
    }

    // Add element to aggregation.
    ++num_elements;
    const int example_embedding_offset = idx * embedding_size;
    const float w = weights_ptr[i];
    current_squares_weight += w * w;
    current_total_weight += w;
    for (int k = 0; k < embedding_size; k++) {
      output_ptr[current_output_offset + k] +=
          value_ptr[example_embedding_offset + k] * w;
    }
  }

  // Finalize last bucket.
  FinalizeAggregation(params->combiner, num_elements, current_total_weight,
                      current_squares_weight, embedding_size,
                      &GetTensorData<float>(output)[current_output_offset]);

  return kTfLiteOk;
}