TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* data = GetInput(context, node, kInputDataTensor);
  const TfLiteTensor* segment_ids =
      GetInput(context, node, kInputSegmentIdsTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  TF_LITE_ENSURE(context,
                 data->type == kTfLiteInt32 || data->type == kTfLiteFloat32);
  TF_LITE_ENSURE_EQ(context, segment_ids->type, kTfLiteInt32);

  if (!IsConstantTensor(data) || !IsConstantTensor(segment_ids)) {
    SetTensorToDynamic(output);
    return kTfLiteOk;
  }

  return ResizeOutputTensor(context, data, segment_ids, output);
}