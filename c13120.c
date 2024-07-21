inline void AveragePool(const float* input_data, const Dims<4>& input_dims,
                        int stride_width, int stride_height, int pad_width,
                        int pad_height, int kwidth, int kheight,
                        float output_activation_min,
                        float output_activation_max, float* output_data,
                        const Dims<4>& output_dims) {
  tflite::PoolParams params;
  params.stride_height = stride_height;
  params.stride_width = stride_width;
  params.filter_height = kheight;
  params.filter_width = kwidth;
  params.padding_values.height = pad_height;
  params.padding_values.width = pad_width;
  params.float_activation_min = output_activation_min;
  params.float_activation_max = output_activation_max;
  AveragePool(params, DimsToShape(input_dims), input_data,
              DimsToShape(output_dims), output_data);
}