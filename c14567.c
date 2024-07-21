TfLiteStatus LogicalImpl(TfLiteContext* context, TfLiteNode* node,
                         bool (*func)(bool, bool)) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  const TfLiteTensor* input1 = GetInput(context, node, kInputTensor1);
  const TfLiteTensor* input2 = GetInput(context, node, kInputTensor2);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  if (data->requires_broadcast) {
    reference_ops::BroadcastBinaryFunction4DSlow<bool, bool, bool>(
        GetTensorShape(input1), GetTensorData<bool>(input1),
        GetTensorShape(input2), GetTensorData<bool>(input2),
        GetTensorShape(output), GetTensorData<bool>(output), func);
  } else {
    reference_ops::BinaryFunction<bool, bool, bool>(
        GetTensorShape(input1), GetTensorData<bool>(input1),
        GetTensorShape(input2), GetTensorData<bool>(input2),
        GetTensorShape(output), GetTensorData<bool>(output), func);
  }

  return kTfLiteOk;
}