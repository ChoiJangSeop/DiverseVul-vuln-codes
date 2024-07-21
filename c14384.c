TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  int num_inputs = NumInputs(node);
  TF_LITE_ENSURE(context, num_inputs >= 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input1 = GetInput(context, node, kInputTensor1);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  output->type = input1->type;

  // Check that all input tensors have the same shape and type.
  for (int i = kInputTensor1 + 1; i < num_inputs; ++i) {
    const TfLiteTensor* input = GetInput(context, node, i);
    TF_LITE_ENSURE(context, HaveSameShapes(input1, input));
    TF_LITE_ENSURE_TYPES_EQ(context, input1->type, input->type);
  }

  // Use the first input node's dimension to be the dimension of the output
  // node.
  TfLiteIntArray* input1_dims = input1->dims;
  TfLiteIntArray* output_dims = TfLiteIntArrayCopy(input1_dims);
  return context->ResizeTensor(context, output, output_dims);
}