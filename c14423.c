TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  const TfLiteTensor* input1 = GetInput(context, node, kInputTensor1);
  const TfLiteTensor* input2 = GetInput(context, node, kInputTensor2);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (output->type) {
    case kTfLiteInt32: {
      // TensorFlow does not support negative for int32.
      TF_LITE_ENSURE_OK(context, CheckValue(context, input2));
      PowImpl<int32_t>(input1, input2, output, data->requires_broadcast);
      break;
    }
    case kTfLiteFloat32: {
      PowImpl<float>(input1, input2, output, data->requires_broadcast);
      break;
    }
    default: {
      context->ReportError(context, "Unsupported data type: %d", output->type);
      return kTfLiteError;
    }
  }
  return kTfLiteOk;
}