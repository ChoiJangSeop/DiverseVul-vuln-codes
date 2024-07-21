TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const auto* params =
      reinterpret_cast<const TfLiteGatherParams*>(node->builtin_data);
  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensor, &input));
  const TfLiteTensor* positions;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputPositions, &positions));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  if (positions->type == kTfLiteInt32) {
    switch (input->type) {
      case kTfLiteFloat32:
        return Gather<float, int32_t>(*params, input, positions, output);
      case kTfLiteUInt8:
        return Gather<uint8_t, int32_t>(*params, input, positions, output);
      case kTfLiteInt8:
        return Gather<int8_t, int32_t>(*params, input, positions, output);
      case kTfLiteInt16:
        return Gather<int16_t, int32_t>(*params, input, positions, output);
      case kTfLiteInt32:
        return Gather<int32_t, int32_t>(*params, input, positions, output);
      case kTfLiteInt64:
        return Gather<int64_t, int32_t>(*params, input, positions, output);
      case kTfLiteBool:
        return Gather<bool, int32_t>(*params, input, positions, output);
      case kTfLiteString:
        return GatherStrings<int32_t>(context, input, positions, output);
      default:
        context->ReportError(context, "Type '%s' is not supported by gather.",
                             TfLiteTypeGetName(input->type));
        return kTfLiteError;
    }
  }
  if (positions->type == kTfLiteInt64) {
    switch (input->type) {
      case kTfLiteFloat32:
        return Gather<float, int64_t>(*params, input, positions, output);
      case kTfLiteUInt8:
        return Gather<uint8_t, int64_t>(*params, input, positions, output);
      case kTfLiteInt8:
        return Gather<int8_t, int64_t>(*params, input, positions, output);
      case kTfLiteInt16:
        return Gather<int16_t, int64_t>(*params, input, positions, output);
      case kTfLiteInt32:
        return Gather<int32_t, int64_t>(*params, input, positions, output);
      case kTfLiteInt64:
        return Gather<int64_t, int64_t>(*params, input, positions, output);
      case kTfLiteBool:
        return Gather<bool, int64_t>(*params, input, positions, output);
      case kTfLiteString:
        return GatherStrings<int64_t>(context, input, positions, output);
      default:
        context->ReportError(context, "Type '%s' is not supported by gather.",
                             TfLiteTypeGetName(input->type));
        return kTfLiteError;
    }
  }
  context->ReportError(context,
                       "Positions of type '%s' are not supported by gather.",
                       TfLiteTypeGetName(positions->type));
  return kTfLiteError;
}