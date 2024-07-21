TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* begin = GetInput(context, node, kBeginTensor);
  const TfLiteTensor* size = GetInput(context, node, kSizeTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_OK(context,
                      ResizeOutputShape(context, input, begin, size, output));
  }

  std::vector<int> begins;
  begins.reserve(kMaxDim);
  std::vector<int> sizes;
  sizes.reserve(kMaxDim);

  for (int i = NumDimensions(input); i < kMaxDim; ++i) {
    begins.push_back(0);
    sizes.push_back(1);
  }

  if (begin->type == kTfLiteInt32) {
    GetBeginAndSizeVectors<int32_t>(NumDimensions(input), begin, size, &begins,
                                    &sizes);
  } else if (begin->type == kTfLiteInt64) {
    GetBeginAndSizeVectors<int64_t>(NumDimensions(input), begin, size, &begins,
                                    &sizes);
  } else {
    context->ReportError(
        context, "Type %d is currently not supported by Slice.", begin->type);
    return kTfLiteError;
  }

  // The original Slice op implementation only accepted 4-D sizes. That
  // constraint is, for the present, maintained here.
  //
  // The dimensions in the kernel used to be in reverse-order, and TFLite
  // arranged the begins and sizes vectors accordingly. This macro incorporates
  // the needed reversing.
#define TF_LITE_SLICE(data_type, kernel_type)                                  \
  {                                                                            \
    TF_LITE_ENSURE_EQ(context, begins.size(), 4);                              \
    TF_LITE_ENSURE_EQ(context, sizes.size(), 4);                               \
    tflite::SliceParams op_params;                                             \
    op_params.begin_count = 4;                                                 \
    op_params.size_count = 4;                                                  \
    for (int i = 0; i < 4; ++i) {                                              \
      op_params.begin[i] = begins[i];                                          \
      op_params.size[i] = sizes[i];                                            \
    }                                                                          \
                                                                               \
    if (kernel_type == kGenericOptimized) {                                    \
      optimized_ops::Slice<data_type>(op_params, GetTensorShape(input), input, \
                                      GetTensorShape(output), output);         \
    } else {                                                                   \
      reference_ops::Slice<data_type>(op_params, GetTensorShape(input), input, \
                                      GetTensorShape(output), output);         \
    }                                                                          \
  }

  switch (input->type) {
    case kTfLiteFloat32:
      TF_LITE_SLICE(float, kernel_type);
      break;
    case kTfLiteInt32:
      TF_LITE_SLICE(int32_t, kernel_type);
      break;
    case kTfLiteInt64:
      TF_LITE_SLICE(int64_t, kernel_type);
      break;
    case kTfLiteInt8:
      TF_LITE_SLICE(int8_t, kernel_type);
      break;
    case kTfLiteInt16:
      TF_LITE_SLICE(int16_t, kernel_type);
      break;
    case kTfLiteUInt8:
      TF_LITE_SLICE(uint8_t, kernel_type);
      break;
    case kTfLiteBool:
      TF_LITE_SLICE(bool, kernel_type);
      break;
    case kTfLiteString:
      TF_LITE_SLICE(string, kernel_type);
      break;
    default:
      context->ReportError(
          context, "Type %d is currently not supported by Slice.", input->type);
      return kTfLiteError;
  }
#undef TF_LITE_SLICE
  return kTfLiteOk;
}