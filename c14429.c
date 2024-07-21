TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* cond_tensor =
      GetInput(context, node, kInputConditionTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_OK(context,
                      ResizeOutputTensor(context, cond_tensor, output));
  }

  TfLiteIntArray* dims = cond_tensor->dims;
  if (dims->size == 0) {
    // Scalar tensors are not supported.
    TF_LITE_KERNEL_LOG(context, "Where op requires condition w/ rank > 0");
    return kTfLiteError;
  }

  reference_ops::SelectTrueCoords(GetTensorShape(cond_tensor),
                                  GetTensorData<bool>(cond_tensor),
                                  GetTensorData<int64_t>(output));
  return kTfLiteOk;
}