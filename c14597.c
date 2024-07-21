TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  // There are two ways in which the 'output' can be made dynamic: it could be
  // a string tensor, or its shape cannot be calculated during Prepare(). In
  // either case, we now have all the information to calculate its shape.
  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_OK(context, ResizeOutput(context, node));
  }

  // Note that string tensors are always "dynamic" in the sense that their size
  // is not known until we have all the content. This applies even when their
  // shape is known ahead of time. As a result, a string tensor is never given
  // any memory by ResizeOutput(), and we need to do it manually here. Since
  // reshape doesn't change the data, the output tensor needs exactly as many
  // bytes as the input tensor.
  if (output->type == kTfLiteString) {
    auto bytes_required = input->bytes;
    TfLiteTensorRealloc(bytes_required, output);
    output->bytes = bytes_required;
  }

  memcpy(output->data.raw, input->data.raw, input->bytes);

  return kTfLiteOk;
}