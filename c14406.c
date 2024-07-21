TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output_index_tensor = GetOutput(context, node, 1);
  TF_LITE_ENSURE_EQ(context, NumElements(output_index_tensor),
                    NumElements(input));

  switch (input->type) {
    case kTfLiteInt8:
      TF_LITE_ENSURE_STATUS(EvalImpl<int8_t>(context, input, node));
      break;
    case kTfLiteInt16:
      TF_LITE_ENSURE_STATUS(EvalImpl<int16_t>(context, input, node));
      break;
    case kTfLiteInt32:
      TF_LITE_ENSURE_STATUS(EvalImpl<int32_t>(context, input, node));
      break;
    case kTfLiteInt64:
      TF_LITE_ENSURE_STATUS(EvalImpl<int64_t>(context, input, node));
      break;
    case kTfLiteFloat32:
      TF_LITE_ENSURE_STATUS(EvalImpl<float>(context, input, node));
      break;
    case kTfLiteUInt8:
      TF_LITE_ENSURE_STATUS(EvalImpl<uint8_t>(context, input, node));
      break;
    default:
      context->ReportError(context, "Currently Unique doesn't support type: %s",
                           TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}