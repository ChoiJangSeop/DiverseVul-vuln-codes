TfLiteStatus NotEqualEval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input1 = GetInput(context, node, kInputTensor1);
  const TfLiteTensor* input2 = GetInput(context, node, kInputTensor2);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  bool requires_broadcast = !HaveSameShapes(input1, input2);
  switch (input1->type) {
    case kTfLiteBool:
      Comparison<bool, reference_ops::NotEqualFn>(input1, input2, output,
                                                  requires_broadcast);
      break;
    case kTfLiteFloat32:
      Comparison<float, reference_ops::NotEqualFn>(input1, input2, output,
                                                   requires_broadcast);
      break;
    case kTfLiteInt32:
      Comparison<int32_t, reference_ops::NotEqualFn>(input1, input2, output,
                                                     requires_broadcast);
      break;
    case kTfLiteInt64:
      Comparison<int64_t, reference_ops::NotEqualFn>(input1, input2, output,
                                                     requires_broadcast);
      break;
    case kTfLiteUInt8:
      ComparisonQuantized<uint8_t, reference_ops::NotEqualFn>(
          input1, input2, output, requires_broadcast);
      break;
    case kTfLiteInt8:
      ComparisonQuantized<int8_t, reference_ops::NotEqualFn>(
          input1, input2, output, requires_broadcast);
      break;
    case kTfLiteString:
      ComparisonString(reference_ops::StringRefNotEqualFn, input1, input2,
                       output, requires_broadcast);
      break;
    default:
      context->ReportError(
          context,
          "Does not support type %d, requires bool|float|int|uint8|string",
          input1->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}