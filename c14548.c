TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* lookup = GetInput(context, node, 0);
  const TfLiteTensor* value = GetInput(context, node, 1);
  TfLiteTensor* output = GetOutput(context, node, 0);
  switch (value->type) {
    case kTfLiteFloat32:
      return EvalSimple(context, node, lookup, value, output);
    case kTfLiteUInt8:
    case kTfLiteInt8:
      if (output->type == kTfLiteFloat32) {
        return EvalHybrid(context, node, lookup, value, output);
      } else {
        return EvalSimple(context, node, lookup, value, output);
      }
    default:
      context->ReportError(context, "Type not currently supported.");
      return kTfLiteError;
  }
}