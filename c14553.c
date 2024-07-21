TfLiteStatus ComparisonPrepareCommon(TfLiteContext* context, TfLiteNode* node,
                                     bool is_string_allowed) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input1 = GetInput(context, node, kInputTensor1);
  const TfLiteTensor* input2 = GetInput(context, node, kInputTensor2);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  // Don't support string.
  if (!is_string_allowed) {
    TF_LITE_ENSURE(context, input1->type != kTfLiteString);
  }
  // Currently only support tensors have the same type.
  TF_LITE_ENSURE_TYPES_EQ(context, input1->type, input2->type);
  output->type = kTfLiteBool;

  bool requires_broadcast = !HaveSameShapes(input1, input2);

  TfLiteIntArray* output_size = nullptr;
  if (requires_broadcast) {
    TF_LITE_ENSURE_OK(context, CalculateShapeForBroadcast(
                                   context, input1, input2, &output_size));
  } else {
    output_size = TfLiteIntArrayCopy(input1->dims);
  }

  return context->ResizeTensor(context, output, output_size);
}