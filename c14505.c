TfLiteStatus LogSoftmaxPrepare(TfLiteContext* context, TfLiteNode* node) {
  LogSoftmaxOpData* data = reinterpret_cast<LogSoftmaxOpData*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);

  if (input->type == kTfLiteUInt8 || input->type == kTfLiteInt8) {
    TF_LITE_ENSURE_EQ(context, output->params.scale, 16.0 / 256);
    static const double kBeta = 1.0;
    if (input->type == kTfLiteUInt8) {
      TF_LITE_ENSURE_EQ(context, output->params.zero_point, 255);
      data->params.table = data->f_table;
      optimized_ops::PopulateSoftmaxLookupTable(&data->params,
                                                input->params.scale, kBeta);
      data->params.zero_point = output->params.zero_point;
      data->params.scale = output->params.scale;
    }
    if (input->type == kTfLiteInt8) {
      TF_LITE_ENSURE_EQ(context, output->params.zero_point, 127);
      static const int kScaledDiffIntegerBits = 5;
      tflite::PreprocessLogSoftmaxScalingExp(
          kBeta, input->params.scale, kScaledDiffIntegerBits,
          &data->input_multiplier, &data->input_left_shift,
          &data->reverse_scaling_divisor, &data->reverse_scaling_right_shift);
      data->reverse_scaling_right_shift *= -1;
      data->diff_min =
          -1.0 * tflite::CalculateInputRadius(kScaledDiffIntegerBits,
                                              data->input_left_shift);
    }
  }

  return context->ResizeTensor(context, output,
                               TfLiteIntArrayCopy(input->dims));
}