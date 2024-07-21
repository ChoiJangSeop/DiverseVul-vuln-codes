TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node, bool is_arg_max) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* axis = GetInput(context, node, kAxis);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_STATUS(ResizeOutput(context, input, axis, output));
  }

#define TF_LITE_ARG_MIN_MAX(data_type, axis_type, output_type) \
  optimized_ops::ArgMinMax(                                    \
      GetTensorShape(input), GetTensorData<data_type>(input),  \
      GetTensorData<axis_type>(axis), GetTensorShape(output),  \
      GetTensorData<output_type>(output),                      \
      GetComparefunction<data_type>(is_arg_max))
  if (axis->type == kTfLiteInt32) {
    switch (output->type) {
      case kTfLiteInt32: {
        switch (input->type) {
          case kTfLiteFloat32:
            TF_LITE_ARG_MIN_MAX(float, int32_t, int32_t);
            break;
          case kTfLiteUInt8:
            TF_LITE_ARG_MIN_MAX(uint8_t, int32_t, int32_t);
            break;
          case kTfLiteInt8:
            TF_LITE_ARG_MIN_MAX(int8_t, int32_t, int32_t);
            break;
          case kTfLiteInt32:
            TF_LITE_ARG_MIN_MAX(int32_t, int32_t, int32_t);
            break;
          default:
            context->ReportError(context,
                                 "Only float32, uint8, int8 and int32 are "
                                 "supported currently, got %s.",
                                 TfLiteTypeGetName(input->type));
            return kTfLiteError;
        }
      } break;
      case kTfLiteInt64: {
        switch (input->type) {
          case kTfLiteFloat32:
            TF_LITE_ARG_MIN_MAX(float, int32_t, int64_t);
            break;
          case kTfLiteUInt8:
            TF_LITE_ARG_MIN_MAX(uint8_t, int32_t, int64_t);
            break;
          case kTfLiteInt8:
            TF_LITE_ARG_MIN_MAX(int8_t, int32_t, int64_t);
            break;
          case kTfLiteInt32:
            TF_LITE_ARG_MIN_MAX(int32_t, int32_t, int64_t);
            break;
          default:
            context->ReportError(context,
                                 "Only float32, uint8, int8 and int32 are "
                                 "supported currently, got %s.",
                                 TfLiteTypeGetName(input->type));
            return kTfLiteError;
        }
      } break;
      default:
        context->ReportError(
            context, "Only int32 and int64 are supported currently, got %s.",
            TfLiteTypeGetName(output->type));
        return kTfLiteError;
    }
  } else {
    switch (output->type) {
      case kTfLiteInt32: {
        switch (input->type) {
          case kTfLiteFloat32:
            TF_LITE_ARG_MIN_MAX(float, int64_t, int32_t);
            break;
          case kTfLiteUInt8:
            TF_LITE_ARG_MIN_MAX(uint8_t, int64_t, int32_t);
            break;
          case kTfLiteInt8:
            TF_LITE_ARG_MIN_MAX(int8_t, int64_t, int32_t);
            break;
          case kTfLiteInt32:
            TF_LITE_ARG_MIN_MAX(int32_t, int64_t, int32_t);
            break;
          default:
            context->ReportError(context,
                                 "Only float32, uint8, int8 and int32 are "
                                 "supported currently, got %s.",
                                 TfLiteTypeGetName(input->type));
            return kTfLiteError;
        }
      } break;
      case kTfLiteInt64: {
        switch (input->type) {
          case kTfLiteFloat32:
            TF_LITE_ARG_MIN_MAX(float, int64_t, int64_t);
            break;
          case kTfLiteUInt8:
            TF_LITE_ARG_MIN_MAX(uint8_t, int64_t, int64_t);
            break;
          case kTfLiteInt8:
            TF_LITE_ARG_MIN_MAX(int8_t, int64_t, int64_t);
            break;
          case kTfLiteInt32:
            TF_LITE_ARG_MIN_MAX(int32_t, int64_t, int64_t);
            break;
          default:
            context->ReportError(context,
                                 "Only float32, uint8, int8 and int32 are "
                                 "supported currently, got %s.",
                                 TfLiteTypeGetName(input->type));
            return kTfLiteError;
        }
      } break;
      default:
        context->ReportError(
            context, "Only int32 and int64 are supported currently, got %s.",
            TfLiteTypeGetName(output->type));
        return kTfLiteError;
    }
  }
#undef TF_LITE_ARG_MIN_MAX

  return kTfLiteOk;
}