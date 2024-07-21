TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* params = GetInput(context, node, kParams);
  const TfLiteTensor* indices = GetInput(context, node, kIndices);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (params->type) {
    case kTfLiteFloat32:
    case kTfLiteUInt8:
    case kTfLiteInt8:
    case kTfLiteInt64:
    case kTfLiteInt32:
    case kTfLiteString:
      break;
    default:
      context->ReportError(
          context, "Params of type '%s' are not supported by gather_nd.",
          TfLiteTypeGetName(params->type));
      return kTfLiteError;
  }
  switch (indices->type) {
    case kTfLiteInt64:
    case kTfLiteInt32:
      break;
    default:
      context->ReportError(
          context, "Indices of type '%s' are not supported by gather_nd.",
          TfLiteTypeGetName(indices->type));
      return kTfLiteError;
  }

  const int params_rank = NumDimensions(params);
  const int indices_rank = NumDimensions(indices);
  const int indices_nd = SizeOfDimension(indices, indices_rank - 1);
  if (params_rank < 1) {
    context->ReportError(context, "Params must be at least a vector.");
    return kTfLiteError;
  }
  if (indices_rank < 1) {
    context->ReportError(context, "Indices must be at least a vector.");
    return kTfLiteError;
  }
  if (indices_nd > params_rank) {
    context->ReportError(
        context, "Index innermost dimension length must be <= params rank.");
    return kTfLiteError;
  }

  // Assign to output the input type.
  output->type = params->type;

  // The result shape is
  // indices.shape[:-1] + params.shape[indices.shape[-1]:]
  const int output_rank = indices_rank + params_rank - indices_nd - 1;
  TfLiteIntArray* output_shape = TfLiteIntArrayCreate(output_rank);
  int output_index = 0;
  for (int i = 0; i < indices_rank - 1; ++i) {
    output_shape->data[output_index++] = indices->dims->data[i];
  }
  for (int i = indices_nd; i < params_rank; ++i) {
    output_shape->data[output_index++] = params->dims->data[i];
  }
  return context->ResizeTensor(context, output, output_shape);
}