TfLiteStatus PreluEval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, 0);
  const TfLiteTensor* alpha = GetInput(context, node, 1);
  TfLiteTensor* output = GetOutput(context, node, 0);
  const PreluOpData* data = reinterpret_cast<PreluOpData*>(node->user_data);
  switch (input->type) {
    case kTfLiteFloat32: {
      if (kernel_type == kGenericOptimized) {
        tflite::ArithmeticParams op_params;
        bool need_broadcast = optimized_ops::ProcessBroadcastShapes(
            GetTensorShape(input), GetTensorShape(alpha), &op_params);
        if (need_broadcast) {
          optimized_ops::BroadcastPReluDispatch(
              op_params, GetTensorShape(input), GetTensorData<float>(input),
              GetTensorShape(alpha), GetTensorData<float>(alpha),
              GetTensorShape(output), GetTensorData<float>(output),
              ApplyPrelu<float>);
        } else {
          const int flat_size =
              MatchingElementsSize(GetTensorShape(input), GetTensorShape(alpha),
                                   GetTensorShape(output));
          optimized_ops::PReluElementWise(
              flat_size, op_params, GetTensorData<float>(alpha),
              GetTensorData<float>(input), GetTensorData<float>(output));
        }
      } else {
        if (data->requires_broadcast) {
          reference_ops::BroadcastBinaryFunction4DSlow<float, float, float>(
              GetTensorShape(input), GetTensorData<float>(input),
              GetTensorShape(alpha), GetTensorData<float>(alpha),
              GetTensorShape(output), GetTensorData<float>(output),
              ApplyPrelu<float>);
        } else {
          reference_ops::BinaryFunction<float, float, float>(
              GetTensorShape(input), GetTensorData<float>(input),
              GetTensorShape(alpha), GetTensorData<float>(alpha),
              GetTensorShape(output), GetTensorData<float>(output),
              ApplyPrelu<float>);
        }
      }
      return kTfLiteOk;
    } break;
    case kTfLiteUInt8: {
      PreluParams op_params;
      op_params.input_offset = -input->params.zero_point;
      op_params.alpha_offset = -alpha->params.zero_point;
      op_params.output_offset = output->params.zero_point;
      op_params.output_multiplier_1 = data->output_multiplier_1;
      op_params.output_shift_1 = data->output_shift_1;
      op_params.output_multiplier_2 = data->output_multiplier_2;
      op_params.output_shift_2 = data->output_shift_2;
      if (data->requires_broadcast) {
        reference_ops::BroadcastPrelu4DSlow(
            op_params, GetTensorShape(input), GetTensorData<uint8_t>(input),
            GetTensorShape(alpha), GetTensorData<uint8_t>(alpha),
            GetTensorShape(output), GetTensorData<uint8_t>(output));
      } else {
        reference_ops::Prelu(
            op_params, GetTensorShape(input), GetTensorData<uint8_t>(input),
            GetTensorShape(alpha), GetTensorData<uint8_t>(alpha),
            GetTensorShape(output), GetTensorData<uint8_t>(output));
      }
      return kTfLiteOk;
    } break;
    case kTfLiteInt8: {
      PreluParams op_params;
      op_params.input_offset = -input->params.zero_point;
      op_params.alpha_offset = -alpha->params.zero_point;
      op_params.output_offset = output->params.zero_point;
      op_params.output_multiplier_1 = data->output_multiplier_1;
      op_params.output_shift_1 = data->output_shift_1;
      op_params.output_multiplier_2 = data->output_multiplier_2;
      op_params.output_shift_2 = data->output_shift_2;
      if (data->requires_broadcast) {
        reference_ops::BroadcastPrelu4DSlow(
            op_params, GetTensorShape(input), GetTensorData<int8_t>(input),
            GetTensorShape(alpha), GetTensorData<int8_t>(alpha),
            GetTensorShape(output), GetTensorData<int8_t>(output));
      } else {
        reference_ops::Prelu(
            op_params, GetTensorShape(input), GetTensorData<int8_t>(input),
            GetTensorShape(alpha), GetTensorData<int8_t>(alpha),
            GetTensorShape(output), GetTensorData<int8_t>(output));
      }
      return kTfLiteOk;
    } break;
    default:
      TF_LITE_KERNEL_LOG(
          context,
          "Only float32 and uint8 and int8 are supported currently, got %d.",
          TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
}