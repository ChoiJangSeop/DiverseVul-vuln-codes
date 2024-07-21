TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 3);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* indices = GetInput(context, node, kIndices);
  const TfLiteTensor* updates = GetInput(context, node, kUpdates);
  const TfLiteTensor* shape = GetInput(context, node, kShape);

  switch (updates->type) {
    case kTfLiteFloat32:
    case kTfLiteUInt8:
    case kTfLiteInt8:
    case kTfLiteInt64:
    case kTfLiteInt32:
      break;
    default:
      context->ReportError(
          context, "Updates of type '%s' are not supported by scatter_nd.",
          TfLiteTypeGetName(updates->type));
      return kTfLiteError;
  }
  if (indices->type != shape->type) {
    context->ReportError(context, "Indices and shape must have the same type.");
    return kTfLiteError;
  }

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  output->type = updates->type;

  if (IsConstantTensor(shape)) {
    switch (indices->type) {
      case kTfLiteInt32:
        TF_LITE_ENSURE_OK(
            context,
            CheckShapes<int32_t>(context, GetTensorShape(indices),
                                 GetTensorShape(updates), GetTensorShape(shape),
                                 GetTensorData<int32_t>(shape)));
        return ResizeOutputTensor<int32_t>(context, shape, output);
      default:
        context->ReportError(
            context, "Indices of type '%s' are not supported by scatter_nd.",
            TfLiteTypeGetName(indices->type));
        return kTfLiteError;
    }
  } else {
    SetTensorToDynamic(output);
    return kTfLiteOk;
  }
}