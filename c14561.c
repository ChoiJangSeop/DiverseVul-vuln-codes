TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input_tensor = GetInput(context, node, 0);
  const TfLiteTensor* padding_matrix = GetInput(context, node, 1);
  TfLiteTensor* output_tensor = GetOutput(context, node, 0);

  TF_LITE_ENSURE_EQ(context, NumDimensions(padding_matrix), 2);
  TF_LITE_ENSURE_EQ(context, SizeOfDimension(padding_matrix, 0),
                    NumDimensions(input_tensor));

  if (!IsConstantTensor(padding_matrix)) {
    SetTensorToDynamic(output_tensor);
    return kTfLiteOk;
  }
  // We have constant padding, so we can infer output size.
  auto output_size = GetPaddedOutputShape(input_tensor, padding_matrix);
  if (output_size == nullptr) {
    return kTfLiteError;
  }
  return context->ResizeTensor(context, output_tensor, output_size.release());
}