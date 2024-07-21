TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* indices = GetInput(context, node, kIndices);
  const TfLiteTensor* updates = GetInput(context, node, kUpdates);
  const TfLiteTensor* shape = GetInput(context, node, kShape);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (indices->type) {
    case kTfLiteInt32:
      return EvalScatterNd<int32_t>(context, indices, updates, shape, output);
    default:
      context->ReportError(
          context, "Indices of type '%s' are not supported by scatter_nd.",
          TfLiteTypeGetName(indices->type));
      return kTfLiteError;
  }
}