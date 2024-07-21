TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteAudioSpectrogramParams*>(node->user_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  TF_LITE_ENSURE(context, params->spectrogram->Initialize(params->window_size,
                                                          params->stride));

  const float* input_data = GetTensorData<float>(input);

  const int64_t sample_count = input->dims->data[0];
  const int64_t channel_count = input->dims->data[1];

  const int64_t output_width = params->spectrogram->output_frequency_channels();

  float* output_flat = GetTensorData<float>(output);

  std::vector<float> input_for_channel(sample_count);
  for (int64_t channel = 0; channel < channel_count; ++channel) {
    float* output_slice =
        output_flat + (channel * params->output_height * output_width);
    for (int i = 0; i < sample_count; ++i) {
      input_for_channel[i] = input_data[i * channel_count + channel];
    }
    std::vector<std::vector<float>> spectrogram_output;
    TF_LITE_ENSURE(context,
                   params->spectrogram->ComputeSquaredMagnitudeSpectrogram(
                       input_for_channel, &spectrogram_output));
    TF_LITE_ENSURE_EQ(context, spectrogram_output.size(),
                      params->output_height);
    TF_LITE_ENSURE(context, spectrogram_output.empty() ||
                                (spectrogram_output[0].size() == output_width));
    for (int row_index = 0; row_index < params->output_height; ++row_index) {
      const std::vector<float>& spectrogram_row = spectrogram_output[row_index];
      TF_LITE_ENSURE_EQ(context, spectrogram_row.size(), output_width);
      float* output_row = output_slice + (row_index * output_width);
      if (params->magnitude_squared) {
        for (int i = 0; i < output_width; ++i) {
          output_row[i] = spectrogram_row[i];
        }
      } else {
        for (int i = 0; i < output_width; ++i) {
          output_row[i] = sqrtf(spectrogram_row[i]);
        }
      }
    }
  }
  return kTfLiteOk;
}