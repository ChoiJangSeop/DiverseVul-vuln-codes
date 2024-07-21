void AveragePool(const float* input_data, const Dims<4>& input_dims, int stride,
                 int pad_width, int pad_height, int filter_width,
                 int filter_height, float* output_data,
                 const Dims<4>& output_dims) {
  AveragePool<Ac>(input_data, input_dims, stride, stride, pad_width, pad_height,
                  filter_width, filter_height, output_data, output_dims);
}