TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  output->type = kTfLiteInt32;

  // By design, the input shape is always known at the time of Prepare, even
  // if the preceding op that generates |input| is dynamic. Thus, we can
  // always compute the rank immediately, without waiting for Eval.
  SetTensorToPersistentRo(output);

  // Rank produces a 0-D int32 Tensor representing the rank of input.
  TfLiteIntArray* output_size = TfLiteIntArrayCreate(0);
  TF_LITE_ENSURE_STATUS(context->ResizeTensor(context, output, output_size));

  TF_LITE_ENSURE_EQ(context, NumDimensions(output), 0);

  // Immediately propagate the known rank to the output tensor. This allows
  // downstream ops that rely on the value to use it during prepare.
  if (output->type == kTfLiteInt32) {
    int32_t* output_data = GetTensorData<int32_t>(output);
    *output_data = NumDimensions(input);
  } else {
    return kTfLiteError;
  }

  return kTfLiteOk;
}