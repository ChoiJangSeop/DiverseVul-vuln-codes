void RunOneAveragePoolTest(const PoolParams& params,
                           const RuntimeShape& input_shape,
                           const int8* input_data,
                           const RuntimeShape& output_shape) {
  const int buffer_size = output_shape.FlatSize();
  std::vector<int8> optimized_averagePool_output(buffer_size);
  std::vector<int8> reference_averagePool_output(buffer_size);

  reference_integer_ops::AveragePool(params, input_shape, input_data,
                                     output_shape,
                                     reference_averagePool_output.data());
  optimized_integer_ops::AveragePool(params, input_shape, input_data,
                                     output_shape,
                                     optimized_averagePool_output.data());

  for (int i = 0; i < buffer_size; i++) {
    EXPECT_TRUE(reference_averagePool_output[i] ==
                optimized_averagePool_output[i]);
  }
}