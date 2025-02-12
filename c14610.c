Status ValidateInput(const OpInputList& indices_list_in,
                     const OpInputList& values_list_in,
                     const OpInputList& shapes_list_in,
                     const OpInputList& dense_list_in) {
  const auto size = indices_list_in.size();
  // Validates indices_list_in OpInputList.
  for (int i = 0; i < size; i++) {
    if (!TensorShapeUtils::IsMatrix(indices_list_in[i].shape())) {
      return errors::InvalidArgument(
          "Input indices should be a matrix but received shape ",
          indices_list_in[i].shape().DebugString(), " at position ", i);
    }
    if (indices_list_in[i].shape().dim_size(1) != 2) {
      return errors::InvalidArgument("Expected D2 of index to be 2 got ",
                                     indices_list_in[i].shape().dim_size(1),
                                     " at position ", i);
    }
  }

  // Validates values_list_in OpInputList.
  if (values_list_in.size() != size) {
    return errors::InvalidArgument("Expected ", size, " input values, got ",
                                   values_list_in.size());
  }
  for (int i = 0; i < size; i++) {
    if (!TensorShapeUtils::IsVector(values_list_in[i].shape())) {
      return errors::InvalidArgument(
          "Input values should be a vector but received shape ",
          values_list_in[i].shape().DebugString(), " at position ", i);
    }
    if (indices_list_in[i].shape().dim_size(0) !=
        values_list_in[i].shape().dim_size(0)) {
      return errors::InvalidArgument(
          "Expected size of values to be ",
          indices_list_in[i].shape().dim_size(0), " got ",
          values_list_in[i].shape().dim_size(0), " at position ", i);
    }
  }

  // Validates shapes_list_in OpInputList
  if (shapes_list_in.size() != size) {
    return errors::InvalidArgument("Expected ", size, " input shapes, got ",
                                   shapes_list_in.size());
  }
  for (int i = 0; i < size; i++) {
    if (!TensorShapeUtils::IsVector(shapes_list_in[i].shape())) {
      return errors::InvalidArgument(
          "Input shapes should be a vector but received shape ",
          shapes_list_in[i].shape().DebugString(), " at position ", i);
    }

    if (shapes_list_in[i].vec<int64>().size() != 2) {
      return errors::InvalidArgument("shape should imply a 2D tensor, but got ",
                                     shapes_list_in[i].shape().DebugString(),
                                     " at position ", i);
    }
  }

  // Validates dense_list_in OpInputList
  for (int i = 0; i < dense_list_in.size(); ++i) {
    if (!TensorShapeUtils::IsMatrix(dense_list_in[i].shape())) {
      return errors::InvalidArgument(
          "Dense inputs should be a matrix but received shape ",
          dense_list_in[i].shape().DebugString(), " at position ", i);
    }
  }

  // Validates batch sizes.  (Note: we do this after validating the input
  // shapes, because CalculateBatchSize() depends on inputs having valid
  // shapes).
  const auto batch_size = CalculateBatchSize(shapes_list_in, dense_list_in);
  for (int i = 0; i < size; i++) {
    if (shapes_list_in[i].vec<int64>()(0) != batch_size) {
      return errors::InvalidArgument("Expected batch size ", batch_size,
                                     " got ", shapes_list_in[i].vec<int64>()(0),
                                     " at position ", i);
    }
  }
  for (int i = 0; i < dense_list_in.size(); ++i) {
    if (dense_list_in[i].dim_size(0) != batch_size) {
      return errors::InvalidArgument("Expected batch size ", batch_size,
                                     " got ", dense_list_in[i].dim_size(0),
                                     " at dense tensor ", i);
    }
  }

  return Status::OK();
}