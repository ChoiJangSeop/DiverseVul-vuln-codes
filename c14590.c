TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (output->type) {
    case kTfLiteFloat32: {
      return ReverseSequenceHelper<float>(context, node);
    }
    case kTfLiteUInt8: {
      return ReverseSequenceHelper<uint8_t>(context, node);
    }
    case kTfLiteInt16: {
      return ReverseSequenceHelper<int16_t>(context, node);
    }
    case kTfLiteInt32: {
      return ReverseSequenceHelper<int32_t>(context, node);
    }
    case kTfLiteInt64: {
      return ReverseSequenceHelper<int64_t>(context, node);
    }
    default: {
      context->ReportError(context,
                           "Type '%s' is not supported by reverse_sequence.",
                           TfLiteTypeGetName(output->type));
      return kTfLiteError;
    }
  }
  return kTfLiteOk;
}  // namespace