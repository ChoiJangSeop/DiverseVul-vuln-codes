TfLiteStatus L2Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLitePoolParams*>(node->builtin_data);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  TfLiteTensor* output = GetOutput(context, node, 0);
  const TfLiteTensor* input = GetInput(context, node, 0);
  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32:
      L2EvalFloat<kernel_type>(context, node, params, data, input, output);
      break;
    case kTfLiteUInt8:
    // We don't have a quantized implementation, so just fall through to the
    // 'default' case.
    default:
      context->ReportError(context, "Type %d not currently supported.",
                           input->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}