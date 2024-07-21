TfLiteStatus EvalImpl(TfLiteContext* context, const TfLiteTensor* input,
                      TfLiteNode* node) {
  // Map from value, to index in the unique elements vector.
  // Note that we prefer to use map than unordered_map as it showed less
  // increase in the binary size.
  std::map<T, int> unique_values;
  TfLiteTensor* output_indexes = GetOutput(context, node, 1);
  std::vector<T> output_values;
  I* indexes = GetTensorData<I>(output_indexes);
  const T* data = GetTensorData<T>(input);
  const int num_elements = NumElements(input);

  for (int i = 0; i < num_elements; ++i) {
    const auto element_it = unique_values.find(data[i]);
    if (element_it != unique_values.end()) {
      indexes[i] = element_it->second;
    } else {
      const int unique_index = unique_values.size();
      unique_values[data[i]] = unique_index;
      indexes[i] = unique_index;
      output_values.push_back(data[i]);
    }
  }
  // Allocate output tensor.
  TfLiteTensor* unique_output = GetOutput(context, node, 0);
  std::unique_ptr<TfLiteIntArray, void (*)(TfLiteIntArray*)> shape(
      TfLiteIntArrayCreate(NumDimensions(input)), TfLiteIntArrayFree);
  shape->data[0] = unique_values.size();
  TF_LITE_ENSURE_STATUS(
      context->ResizeTensor(context, unique_output, shape.release()));
  // Set the values in the output tensor.
  T* output_unique_values = GetTensorData<T>(unique_output);
  for (int i = 0; i < output_values.size(); ++i) {
    output_unique_values[i] = output_values[i];
  }
  return kTfLiteOk;
}