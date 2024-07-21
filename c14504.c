TfLiteStatus Rfft2dHelper(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const float* input_data = GetTensorData<float>(input);
  const TfLiteTensor* fft_length = GetInput(context, node, kFftLengthTensor);
  const int32_t* fft_length_data = GetTensorData<int32_t>(fft_length);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  complex<float>* output_data = GetTensorData<complex<float>>(output);

  int fft_height, fft_width;
  fft_height = fft_length_data[0];
  fft_width = fft_length_data[1];

  // FFT is processed for every slice on the inner most 2 dimensions.
  // Count the number of slices in the input tensor.
  const RuntimeShape input_shape = GetTensorShape(input);
  const int input_dims_count = input_shape.DimensionsCount();
  const auto* input_dims_data = input_shape.DimsData();
  int num_slices = 1;
  for (int i = 0; i < input_dims_count - 2; ++i) {
    num_slices *= input_dims_data[i];
  }

  int input_height = input_dims_data[input_dims_count - 2];
  int input_width = input_dims_data[input_dims_count - 1];
  int input_slice_size = input_height * input_width;
  int output_slice_size = fft_height * (fft_width / 2 + 1);

  // Create input/output buffer for FFT
  double** fft_input_output = new double*[fft_height];
  for (int i = 0; i < fft_height; ++i) {
    fft_input_output[i] = new double[fft_width + 2];
  }

  // Get buffer for integer working area.
  TfLiteTensor* fft_integer_working_area =
      GetTemporary(context, node, kFftIntegerWorkingAreaTensor);
  int* fft_integer_working_area_data =
      GetTensorData<int>(fft_integer_working_area);

  // Get buffer for double working area.
  TfLiteTensor* fft_double_working_area =
      GetTemporary(context, node, kFftDoubleWorkingAreaTensor);
  // Get double value out of the memory of fft_double_working_area_data.
  double* fft_double_working_area_data = reinterpret_cast<double*>(
      GetTensorData<int64_t>(fft_double_working_area));

  // Process every slice in the input buffer
  for (int i = 0; i < num_slices; ++i) {
    PrepareInputBuffer(input_data, input_height, input_width, fft_height,
                       fft_width, fft_input_output);
    memset(fft_integer_working_area_data, 0, fft_integer_working_area->bytes);
    memset(fft_double_working_area_data, 0, fft_double_working_area->bytes);
    Rfft2dImpl(fft_height, fft_width, fft_input_output,
               fft_integer_working_area_data, fft_double_working_area_data);
    PrepareOutputBuffer(output_data, fft_height, fft_width, fft_input_output);
    input_data += input_slice_size;
    output_data += output_slice_size;
  }

  // Delete the input buffer
  for (int i = 0; i < fft_height; ++i) {
    delete[] fft_input_output[i];
  }
  delete[] fft_input_output;

  return kTfLiteOk;
}