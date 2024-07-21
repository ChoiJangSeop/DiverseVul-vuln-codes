void AveragePool(const float* input_data, const Dims<4>& input_dims,
                 int stride_width, int stride_height, int pad_width,
                 int pad_height, int kwidth, int kheight, float* output_data,
                 const Dims<4>& output_dims) {
  float output_activation_min, output_activation_max;
  GetActivationMinMax(Ac, &output_activation_min, &output_activation_max);

  AveragePool(input_data, input_dims, stride_width, stride_height, pad_width,
              pad_height, kwidth, kheight, output_activation_min,
              output_activation_max, output_data, output_dims);
}