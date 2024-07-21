TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* indices = GetInput(context, node, kIndicesTensor);
  const TfLiteTensor* values = GetInput(context, node, kValueInputTensor);

  switch (values->type) {
    case kTfLiteFloat32:
      return EvalForIndexType<float>(context, node, indices);
    case kTfLiteInt32:
      return EvalForIndexType<int32_t>(context, node, indices);
    case kTfLiteInt64:
      return EvalForIndexType<int64_t>(context, node, indices);
    case kTfLiteInt8:
      return EvalForIndexType<int8_t>(context, node, indices);
    case kTfLiteUInt8:
      return EvalForIndexType<uint8_t>(context, node, indices);
    default:
      TF_LITE_KERNEL_LOG(
          context,
          "Value type %s is currently not supported by sparse to dense.",
          TfLiteTypeGetName(values->type));
      return kTfLiteError;
  }
}