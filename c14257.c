void TestGatherNd(int* param_dims, const ParamType* param_data, int* index_dims,
                  const IndexType* index_data, int* output_dims,
                  ParamType* output_data,
                  const ParamType* expected_output_data) {
  TfLiteIntArray* pdims = IntArrayFromInts(param_dims);
  TfLiteIntArray* idims = IntArrayFromInts(index_dims);
  TfLiteIntArray* odims = IntArrayFromInts(output_dims);

  constexpr int inputs_size = 2;
  constexpr int outputs_size = 1;
  constexpr int tensors_size = inputs_size + outputs_size;
  TfLiteTensor tensors[tensors_size] = {
      CreateTensor(param_data, pdims),
      CreateTensor(index_data, idims),
      CreateTensor(output_data, odims),
  };
  int inputs_array_data[] = {2, 0, 1};
  TfLiteIntArray* inputs_array = IntArrayFromInts(inputs_array_data);
  int outputs_array_data[] = {1, 2};
  TfLiteIntArray* outputs_array = IntArrayFromInts(outputs_array_data);

  const TfLiteRegistration registration = Register_GATHER_ND();
  micro::KernelRunner runner(registration, tensors, tensors_size, inputs_array,
                             outputs_array, /*builtin_data=*/nullptr);
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, runner.InitAndPrepare());
  TF_LITE_MICRO_EXPECT_EQ(kTfLiteOk, runner.Invoke());

  // The output tensor's data and shape have been updated by the kernel.
  TfLiteTensor* actual_output_tensor = &tensors[2];
  TfLiteIntArray* actual_output_dims = actual_output_tensor->dims;
  const int output_size = ElementCount(*actual_output_dims);
  for (int i = 0; i < output_size; ++i) {
    TF_LITE_MICRO_EXPECT_EQ(expected_output_data[i], output_data[i]);
  }
}