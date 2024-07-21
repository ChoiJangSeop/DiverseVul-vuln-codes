TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteAudioSpectrogramParams*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  TF_LITE_ENSURE_EQ(context, NumDimensions(input), 2);

  TF_LITE_ENSURE_TYPES_EQ(context, output->type, kTfLiteFloat32);
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);

  TF_LITE_ENSURE(context, params->spectrogram->Initialize(params->window_size,
                                                          params->stride));
  const int64_t sample_count = input->dims->data[0];
  const int64_t length_minus_window = (sample_count - params->window_size);
  if (length_minus_window < 0) {
    params->output_height = 0;
  } else {
    params->output_height = 1 + (length_minus_window / params->stride);
  }
  TfLiteIntArray* output_size = TfLiteIntArrayCreate(3);
  output_size->data[0] = input->dims->data[1];
  output_size->data[1] = params->output_height;
  output_size->data[2] = params->spectrogram->output_frequency_channels();

  return context->ResizeTensor(context, output, output_size);
}