TfLiteStatus SigmoidEval(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  switch (input->type) {
    case kTfLiteFloat32: {
      if (kernel_type == kReference) {
        reference_ops::Logistic(
            GetTensorShape(input), GetTensorData<float>(input),
            GetTensorShape(output), GetTensorData<float>(output));
      } else {
        optimized_ops::Logistic(
            GetTensorShape(input), GetTensorData<float>(input),
            GetTensorShape(output), GetTensorData<float>(output));
      }
      break;
    }
    case kTfLiteInt16: {
      LogisticParams params;
      if (kernel_type == kReference || (data->input_multiplier > 0)) {
        const int size =
            MatchingFlatSize(GetTensorShape(input), GetTensorShape(output));

        reference_integer_ops::Logistic(data->input_multiplier, size,
                                        GetTensorData<int16_t>(input),
                                        GetTensorData<int16_t>(output));
      } else {
        optimized_ops::Logistic(
            params, GetTensorShape(input), GetTensorData<int16_t>(input),
            GetTensorShape(output), GetTensorData<int16_t>(output));
      }
      break;
    }
    case kTfLiteUInt8: {
      if (kernel_type == kFixedPointOptimized) {
        LogisticParams params;
        params.input_zero_point = input->params.zero_point;
        params.input_range_radius = data->input_range_radius;
        params.input_multiplier = data->input_multiplier;
        params.input_left_shift = data->input_left_shift;
        optimized_ops::Logistic16bitPrecision(
            params, GetTensorShape(input), GetTensorData<uint8_t>(input),
            GetTensorShape(output), GetTensorData<uint8_t>(output));
      } else {
        EvalUsingLookupTable(data, input, output);
      }
      break;
    }
    case kTfLiteInt8: {
      if (kernel_type == kFixedPointOptimized) {
        LogisticParams params;
        params.input_zero_point = input->params.zero_point;
        params.input_range_radius = data->input_range_radius;
        params.input_multiplier = data->input_multiplier;
        params.input_left_shift = data->input_left_shift;
        optimized_ops::Logistic16bitPrecision(
            params, GetTensorShape(input), GetTensorData<int8_t>(input),
            GetTensorShape(output), GetTensorData<int8_t>(output));
      } else {
        EvalUsingLookupTable(data, input, output);
      }
      break;
    }
    default:
      TF_LITE_KERNEL_LOG(context,
                         "Only float32, uint8, int16 and int8 are supported "
                         "currently, got %s.",
                         TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}