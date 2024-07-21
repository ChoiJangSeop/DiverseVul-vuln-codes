TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteIntArray* input_dims = input->dims;
  int input_dims_size = input_dims->size;
  TF_LITE_ENSURE(context, input_dims_size >= 1);

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  // Resize the output tensor.
  TfLiteIntArray* output_shape = TfLiteIntArrayCreate(input_dims_size + 1);
  for (int i = 0; i < input_dims_size; i++) {
    output_shape->data[i] = input_dims->data[i];
  }
  // Last dimension in the output is the same as the last dimension in the
  // input.
  output_shape->data[input_dims_size] = input_dims->data[input_dims_size - 1];
  output->type = input->type;
  TF_LITE_ENSURE_OK(context,
                    context->ResizeTensor(context, output, output_shape));

  return kTfLiteOk;
}