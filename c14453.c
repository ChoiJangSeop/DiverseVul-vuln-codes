TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteIntArray* input_dims = input->dims;
  int input_dims_size = input_dims->size;
  TF_LITE_ENSURE(context, input_dims_size >= 2);

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  TfLiteIntArray* output_shape = TfLiteIntArrayCreate(input_dims_size);
  for (int i = 0; i < input_dims_size; i++) {
    output_shape->data[i] = input_dims->data[i];
  }

  // Resize the output tensor to the same size as the input tensor.
  output->type = input->type;
  TF_LITE_ENSURE_OK(context,
                    context->ResizeTensor(context, output, output_shape));

  return kTfLiteOk;
}