TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  // Check type and shape of the input tensor
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TF_LITE_ENSURE(context, NumDimensions(input) >= 2);
  if (input->type != kTfLiteFloat32) {
    context->ReportError(context,
                         "Type '%s' for input is not supported by rfft2d.",
                         TfLiteTypeGetName(input->type));
    return kTfLiteError;
  }

  // Check type and shape of the fft_length tensor
  const TfLiteTensor* fft_length = GetInput(context, node, kFftLengthTensor);
  const RuntimeShape fft_length_shape = GetTensorShape(fft_length);

  TF_LITE_ENSURE_EQ(context, NumDimensions(fft_length), 1);
  TF_LITE_ENSURE_EQ(context, fft_length_shape.Dims(0), 2);
  if (fft_length->type != kTfLiteInt32) {
    context->ReportError(context,
                         "Type '%s' for fft_length is not supported by rfft2d.",
                         TfLiteTypeGetName(fft_length->type));
    return kTfLiteError;
  }

  // Setup temporary tensors for fft computation.
  TF_LITE_ENSURE_STATUS(InitTemporaryTensors(context, node));

  // Set output type
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  output->type = kTfLiteComplex64;

  // Exit early if fft_length is a non-const tensor. Set output tensor and
  // temporary tensors to dynamic, so that their tensor sizes can be determined
  // in Eval.
  if (!IsConstantTensor(fft_length)) {
    TfLiteTensor* fft_integer_working_area =
        GetTemporary(context, node, kFftIntegerWorkingAreaTensor);
    TfLiteTensor* fft_double_working_area =
        GetTemporary(context, node, kFftDoubleWorkingAreaTensor);
    SetTensorToDynamic(fft_integer_working_area);
    SetTensorToDynamic(fft_double_working_area);
    SetTensorToDynamic(output);
    return kTfLiteOk;
  }

  TF_LITE_ENSURE_STATUS(ResizeOutputandTemporaryTensors(context, node));
  return kTfLiteOk;
}