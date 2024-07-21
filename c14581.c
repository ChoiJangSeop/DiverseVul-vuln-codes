TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteMfccParams*>(node->user_data);

  const TfLiteTensor* input_wav = GetInput(context, node, kInputTensorWav);
  const TfLiteTensor* input_rate = GetInput(context, node, kInputTensorRate);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  const int32 sample_rate = *GetTensorData<int>(input_rate);

  const int spectrogram_channels = input_wav->dims->data[2];
  const int spectrogram_samples = input_wav->dims->data[1];
  const int audio_channels = input_wav->dims->data[0];

  internal::Mfcc mfcc;
  mfcc.set_upper_frequency_limit(params->upper_frequency_limit);
  mfcc.set_lower_frequency_limit(params->lower_frequency_limit);
  mfcc.set_filterbank_channel_count(params->filterbank_channel_count);
  mfcc.set_dct_coefficient_count(params->dct_coefficient_count);

  mfcc.Initialize(spectrogram_channels, sample_rate);

  const float* spectrogram_flat = GetTensorData<float>(input_wav);
  float* output_flat = GetTensorData<float>(output);

  for (int audio_channel = 0; audio_channel < audio_channels; ++audio_channel) {
    for (int spectrogram_sample = 0; spectrogram_sample < spectrogram_samples;
         ++spectrogram_sample) {
      const float* sample_data =
          spectrogram_flat +
          (audio_channel * spectrogram_samples * spectrogram_channels) +
          (spectrogram_sample * spectrogram_channels);
      std::vector<double> mfcc_input(sample_data,
                                     sample_data + spectrogram_channels);
      std::vector<double> mfcc_output;
      mfcc.Compute(mfcc_input, &mfcc_output);
      TF_LITE_ENSURE_EQ(context, params->dct_coefficient_count,
                        mfcc_output.size());
      float* output_data = output_flat +
                           (audio_channel * spectrogram_samples *
                            params->dct_coefficient_count) +
                           (spectrogram_sample * params->dct_coefficient_count);
      for (int i = 0; i < params->dct_coefficient_count; ++i) {
        output_data[i] = mfcc_output[i];
      }
    }
  }

  return kTfLiteOk;
}