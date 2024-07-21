TfLiteStatus SparseToDenseImpl(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* indices = GetInput(context, node, kIndicesTensor);
  const TfLiteTensor* output_shape =
      GetInput(context, node, kOutputShapeTensor);
  const TfLiteTensor* values = GetInput(context, node, kValueInputTensor);
  const TfLiteTensor* default_value =
      GetInput(context, node, kDefaultValueTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_OK(context,
                      ResizeOutputShape(context, output_shape, output));
  }

  const int num_indices = SizeOfDimension(indices, 0);
  const bool value_is_scalar = NumDimensions(values) == 0;
  std::vector<std::vector<TI>> indices_vector;
  indices_vector.reserve(num_indices);
  TF_LITE_ENSURE_OK(context, GetIndicesVector<TI>(context, indices, num_indices,
                                                  &indices_vector));
  reference_ops::SparseToDense(indices_vector, GetTensorData<T>(values),
                               *GetTensorData<T>(default_value),
                               value_is_scalar, GetTensorShape(output),
                               GetTensorData<T>(output));

  return kTfLiteOk;
}