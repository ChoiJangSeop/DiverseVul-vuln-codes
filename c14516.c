TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  if (output->type == kTfLiteFloat32) {
    EvalAddN<float>(context, node);
  } else if (output->type == kTfLiteInt32) {
    EvalAddN<int32_t>(context, node);
  } else {
    context->ReportError(context,
                         "AddN only supports FLOAT32|INT32 now, got %s.",
                         TfLiteTypeGetName(output->type));
    return kTfLiteError;
  }
  return kTfLiteOk;
}