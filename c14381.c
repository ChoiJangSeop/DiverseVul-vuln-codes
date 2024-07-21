TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* fft_length = GetInput(context, node, kFftLengthTensor);
  const int32_t* fft_length_data = GetTensorData<int32_t>(fft_length);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  if (output->type != kTfLiteComplex64) {
    context->ReportError(context,
                         "Type '%s' for output is not supported by rfft2d.",
                         TfLiteTypeGetName(output->type));
    return kTfLiteError;
  }

  // Resize the output tensor if the fft_length tensor is not constant.
  // Otherwise, check if the output shape is correct.
  if (!IsConstantTensor(fft_length)) {
    TF_LITE_ENSURE_STATUS(ResizeOutputandTemporaryTensors(context, node));
  } else {
    int num_dims_output = NumDimensions(output);
    const RuntimeShape output_shape = GetTensorShape(output);
    TF_LITE_ENSURE_EQ(context, num_dims_output, NumDimensions(input));
    TF_LITE_ENSURE(context, num_dims_output >= 2);
    TF_LITE_ENSURE_EQ(context, output_shape.Dims(num_dims_output - 2),
                      fft_length_data[0]);
    TF_LITE_ENSURE_EQ(context, output_shape.Dims(num_dims_output - 1),
                      fft_length_data[1] / 2 + 1);
  }

  return Rfft2dHelper(context, node);
}