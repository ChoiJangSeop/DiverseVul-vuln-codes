void EvalAddN(TfLiteContext* context, TfLiteNode* node) {
  // TODO(haoliang): Initialize all_inputs only once during init.
  VectorOfTensors<T> all_inputs(*context, *node->inputs);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  int num_inputs = NumInputs(node);
  const TfLiteTensor* input1 = GetInput(context, node, kInputTensor1);
  reference_ops::AddN<T>(GetTensorShape(input1), num_inputs, all_inputs.data(),
                         GetTensorData<T>(output));
}