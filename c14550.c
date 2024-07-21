TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = static_cast<OpData*>(node->user_data);

  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);

  const RuntimeShape input_shape = GetTensorShape(input);
  const RuntimeShape output_shape = GetTensorShape(output);

  switch (input->type) {
    case kTfLiteFloat32: {
      // Float to int8, uint8, int16.
      tflite::QuantizationParams op_params;
      op_params.zero_point = output->params.zero_point;
      op_params.scale = output->params.scale;
      const float* input_data = GetTensorData<float>(input);
      switch (output->type) {
        case kTfLiteInt8:
          AffineQuantize<kernel_type>(op_params, input_shape, input_data,
                                      output_shape,
                                      GetTensorData<int8_t>(output));
          return kTfLiteOk;
        case kTfLiteUInt8:
          AffineQuantize<kernel_type>(op_params, input_shape, input_data,
                                      output_shape,
                                      GetTensorData<uint8_t>(output));
          return kTfLiteOk;
        case kTfLiteInt16:
          AffineQuantize<kernel_type>(op_params, input_shape, input_data,
                                      output_shape,
                                      GetTensorData<int16_t>(output));
          return kTfLiteOk;
        default:
          ReportError(context, input->type, output->type);
          return kTfLiteError;
      }
    }
    case kTfLiteInt16: {
      // int16 to int8 or int16.
      switch (output->type) {
        case kTfLiteInt8:
          Requantize<kernel_type>(GetTensorData<int16_t>(input),
                                  MatchingFlatSize(input_shape, output_shape),
                                  data->output_multiplier, data->output_shift,
                                  input->params.zero_point,
                                  output->params.zero_point,
                                  GetTensorData<int8_t>(output));
          return kTfLiteOk;
        case kTfLiteInt16:
          Requantize<kernel_type>(GetTensorData<int16_t>(input),
                                  MatchingFlatSize(input_shape, output_shape),
                                  data->output_multiplier, data->output_shift,
                                  input->params.zero_point,
                                  output->params.zero_point,
                                  GetTensorData<int16_t>(output));
          return kTfLiteOk;
        default:
          ReportError(context, input->type, output->type);
          return kTfLiteError;
      }
    }
    case kTfLiteInt8: {
      // int8 to int8, uint8.
      const int32_t size = MatchingFlatSize(input_shape, output_shape);
      const int8_t* input_data = GetTensorData<int8_t>(input);
      switch (output->type) {
        case kTfLiteInt8:
          Requantize<kernel_type>(input_data, size, data->output_multiplier,
                                  data->output_shift, input->params.zero_point,
                                  output->params.zero_point,
                                  GetTensorData<int8_t>(output));
          return kTfLiteOk;
        case kTfLiteUInt8:
          Requantize<kernel_type>(input_data, size, data->output_multiplier,
                                  data->output_shift, input->params.zero_point,
                                  output->params.zero_point,
                                  GetTensorData<uint8_t>(output));
          return kTfLiteOk;
        default:
          ReportError(context, input->type, output->type);
          return kTfLiteError;
      }
    }
    case kTfLiteUInt8: {
      // uint8 to int8, uint8.
      const int32_t size = MatchingFlatSize(input_shape, output_shape);
      const uint8_t* input_data = GetTensorData<uint8_t>(input);
      switch (output->type) {
        case kTfLiteInt8:
          Requantize<kernel_type>(input_data, size, data->output_multiplier,
                                  data->output_shift, input->params.zero_point,
                                  output->params.zero_point,
                                  GetTensorData<int8_t>(output));
          return kTfLiteOk;
        case kTfLiteUInt8:
          Requantize<kernel_type>(input_data, size, data->output_multiplier,
                                  data->output_shift, input->params.zero_point,
                                  output->params.zero_point,
                                  GetTensorData<uint8_t>(output));
          return kTfLiteOk;
        default:
          ReportError(context, input->type, output->type);
          return kTfLiteError;
      }
    }
    default:
      ReportError(context, input->type, output->type);
      return kTfLiteError;
  }
}