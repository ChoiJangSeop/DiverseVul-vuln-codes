TfLiteStatus TanhEval(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  switch (input->type) {
    case kTfLiteFloat32: {
      if (kernel_type == kReference) {
        reference_ops::Tanh(GetTensorShape(input), GetTensorData<float>(input),
                            GetTensorShape(output),
                            GetTensorData<float>(output));
      } else {
        optimized_ops::Tanh(GetTensorShape(input), GetTensorData<float>(input),
                            GetTensorShape(output),
                            GetTensorData<float>(output));
      }
      return kTfLiteOk;
    } break;
    case kTfLiteInt16: {
      TanhParams params;
      params.input_left_shift = data->input_left_shift;
      if (kernel_type == kReference || (data->input_multiplier > 0)) {
        reference_integer_ops::Tanh(
            data->input_multiplier, data->input_left_shift,
            GetTensorShape(input), GetTensorData<int16_t>(input),
            GetTensorShape(output), GetTensorData<int16_t>(output));
      } else {
        optimized_ops::Tanh(
            params, GetTensorShape(input), GetTensorData<int16_t>(input),
            GetTensorShape(output), GetTensorData<int16_t>(output));
      }
      return kTfLiteOk;
    } break;
    case kTfLiteUInt8: {
      if (kernel_type == kFixedPointOptimized) {
        TanhParams params;
        params.input_zero_point = input->params.zero_point;
        params.input_range_radius = data->input_range_radius;
        params.input_multiplier = data->input_multiplier;
        params.input_left_shift = data->input_left_shift;
        optimized_ops::Tanh16bitPrecision(
            params, GetTensorShape(input), GetTensorData<uint8_t>(input),
            GetTensorShape(output), GetTensorData<uint8_t>(output));
      } else {
        EvalUsingLookupTable(data, input, output);
      }
      return kTfLiteOk;
    } break;
    case kTfLiteInt8: {
      if (kernel_type == kFixedPointOptimized) {
        TanhParams params;
        params.input_zero_point = input->params.zero_point;
        params.input_range_radius = data->input_range_radius;
        params.input_multiplier = data->input_multiplier;
        params.input_left_shift = data->input_left_shift;
        optimized_ops::Tanh16bitPrecision(
            params, GetTensorShape(input), GetTensorData<int8_t>(input),
            GetTensorShape(output), GetTensorData<int8_t>(output));
      } else {
        EvalUsingLookupTable(data, input, output);
      }
      return kTfLiteOk;
    } break;
    default:
      TF_LITE_KERNEL_LOG(context,
                         "Only float32, uint8, int16 and int8 are supported "
                         "currently, got %s.",
                         TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
}