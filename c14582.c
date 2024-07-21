TfLiteStatus SoftmaxEval(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteSoftmaxParams*>(node->builtin_data);
  SoftmaxOpData* data = reinterpret_cast<SoftmaxOpData*>(node->user_data);

  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);

  switch (input->type) {
    case kTfLiteFloat32: {
      return SoftmaxFloat(context, input, output, params);
    }
    case kTfLiteUInt8: {
      switch (output->type) {
        case kTfLiteUInt8:
          return SoftmaxQuantized<uint8_t, uint8_t>(context, input, output,
                                                    data);
        case kTfLiteInt16:
          return SoftmaxQuantized<uint8_t, int16_t>(context, input, output,
                                                    data);
        default:
          TF_LITE_KERNEL_LOG(context,
                             "Only uint8_t and int16_t outputs are supported "
                             "with uint8_t inputs currently, got %s.",
                             TfLiteTypeGetName(output->type));
          return kTfLiteError;
      }
    }
    case kTfLiteInt8: {
      switch (output->type) {
        case kTfLiteInt8:
          return SoftmaxQuantized<int8_t, int8_t>(context, input, output, data);
        case kTfLiteInt16:
          return SoftmaxQuantized<int8_t, int16_t>(context, input, output,
                                                   data);
        default:
          TF_LITE_KERNEL_LOG(context,
                             "Only int8_t and int16_t outputs are supported "
                             "with int8_t inputs currently, got %s.",
                             TfLiteTypeGetName(output->type));
          return kTfLiteError;
      }
    }
    case kTfLiteInt16: {
      return SoftmaxQuantized<int16_t, int16_t>(context, input, output, data);
    }

    default:
      TF_LITE_KERNEL_LOG(context,
                         "Only float32, uint8_t, Int8_t, Int16_t are supported "
                         "currently, got %s.",
                         TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
}