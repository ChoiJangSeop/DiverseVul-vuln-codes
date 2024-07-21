TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  const TfLiteTensor* multipliers = GetInput(context, node, kInputMultipliers);

  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_OK(context, ResizeOutput(context, node));
  }

  switch (output->type) {
    case kTfLiteFloat32:
      Tile<float>(*(input->dims), input, multipliers, output);
      break;
    case kTfLiteUInt8:
      Tile<uint8_t>(*(input->dims), input, multipliers, output);
      break;
    case kTfLiteInt32:
      Tile<int32_t>(*(input->dims), input, multipliers, output);
      break;
    case kTfLiteInt64:
      Tile<int64_t>(*(input->dims), input, multipliers, output);
      break;
    case kTfLiteString: {
      DynamicBuffer buffer;
      TileString(*(input->dims), input, multipliers, &buffer, output);
      buffer.WriteToTensor(output, /*new_shape=*/nullptr);
      break;
    }
    case kTfLiteBool:
      Tile<bool>(*(input->dims), input, multipliers, output);
      break;
    default:
      context->ReportError(context, "Type '%s' is not supported by tile.",
                           TfLiteTypeGetName(output->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}