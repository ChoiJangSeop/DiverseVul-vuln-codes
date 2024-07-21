TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* axis_tensor = GetInput(context, node, kAxisTensor);
  int axis = GetTensorData<int32_t>(axis_tensor)[0];
  const int rank = NumDimensions(input);
  if (axis < 0) {
    axis += rank;
  }

  TF_LITE_ENSURE(context, axis >= 0 && axis < rank);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (output->type) {
    case kTfLiteFloat32: {
      reference_ops::Reverse<float>(
          axis, GetTensorShape(input), GetTensorData<float>(input),
          GetTensorShape(output), GetTensorData<float>(output));
      break;
    }
    case kTfLiteUInt8: {
      reference_ops::Reverse<uint8_t>(
          axis, GetTensorShape(input), GetTensorData<uint8_t>(input),
          GetTensorShape(output), GetTensorData<uint8_t>(output));
      break;
    }
    case kTfLiteInt16: {
      reference_ops::Reverse<int16_t>(
          axis, GetTensorShape(input), GetTensorData<int16_t>(input),
          GetTensorShape(output), GetTensorData<int16_t>(output));
      break;
    }
    case kTfLiteInt32: {
      reference_ops::Reverse<int32_t>(
          axis, GetTensorShape(input), GetTensorData<int32_t>(input),
          GetTensorShape(output), GetTensorData<int32_t>(output));
      break;
    }
    case kTfLiteInt64: {
      reference_ops::Reverse<int64_t>(
          axis, GetTensorShape(input), GetTensorData<int64_t>(input),
          GetTensorShape(output), GetTensorData<int64_t>(output));
      break;
    }
    case kTfLiteBool: {
      reference_ops::Reverse<bool>(
          axis, GetTensorShape(input), GetTensorData<bool>(input),
          GetTensorShape(output), GetTensorData<bool>(output));
      break;
    }
    default: {
      context->ReportError(context, "Type '%s' is not supported by reverse.",
                           TfLiteTypeGetName(output->type));
      return kTfLiteError;
    }
  }

  return kTfLiteOk;
}