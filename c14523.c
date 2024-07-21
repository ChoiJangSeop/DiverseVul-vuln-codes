TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* start = GetInput(context, node, kStartTensor);
  const TfLiteTensor* limit = GetInput(context, node, kLimitTensor);
  const TfLiteTensor* delta = GetInput(context, node, kDeltaTensor);

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_OK(context,
                      ResizeOutput(context, start, limit, delta, output));
  }

  switch (output->type) {
    case kTfLiteInt32: {
      EvalImpl<int32_t>(start, delta, output);
      break;
    }
    case kTfLiteFloat32: {
      EvalImpl<float>(start, delta, output);
      break;
    }
    default: {
      context->ReportError(context, "Unsupported data type: %d", output->type);
      return kTfLiteError;
    }
  }
  return kTfLiteOk;
}