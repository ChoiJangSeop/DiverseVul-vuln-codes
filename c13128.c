inline void AveragePool(const uint8* input_data, const Dims<4>& input_dims,
                        int stride_width, int stride_height, int pad_width,
                        int pad_height, int filter_width, int filter_height,
                        int32 output_activation_min,
                        int32 output_activation_max, uint8* output_data,
                        const Dims<4>& output_dims) {
  tflite::PoolParams params;
  params.stride_height = stride_height;
  params.stride_width = stride_width;
  params.filter_height = filter_height;
  params.filter_width = filter_width;
  params.padding_values.height = pad_height;
  params.padding_values.width = pad_width;
  params.quantized_activation_min = output_activation_min;
  params.quantized_activation_max = output_activation_max;
  AveragePool(params, DimsToShape(input_dims), input_data,
              DimsToShape(output_dims), output_data);
}