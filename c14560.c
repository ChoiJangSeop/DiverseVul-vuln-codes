TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteSVDFParams*>(node->builtin_data);
  OpData* op_data = reinterpret_cast<OpData*>(node->user_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* weights_feature =
      GetInput(context, node, kWeightsFeatureTensor);
  const TfLiteTensor* weights_time =
      GetInput(context, node, kWeightsTimeTensor);
  const TfLiteTensor* bias = GetOptionalInputTensor(context, node, kBiasTensor);

  TfLiteTensor* scratch = GetTemporary(context, node, /*index=*/0);

  TfLiteTensor* state = GetVariableInput(context, node, kStateTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (weights_feature->type) {
    case kTfLiteFloat32: {
      reference_ops::EvalFloatSVDF(context, node, input, weights_feature,
                                   weights_time, bias, params, scratch, state,
                                   output);
      return kTfLiteOk;
      break;
    }
    case kTfLiteUInt8:
    case kTfLiteInt8: {
      if (input->type == kTfLiteFloat32) {
        TfLiteTensor* input_quantized =
            GetTemporary(context, node, /*index=*/1);
        TfLiteTensor* scaling_factors =
            GetTemporary(context, node, /*index=*/2);
        TfLiteTensor* float_weights_time =
            GetTemporary(context, node, /*index=*/3);
        TfLiteTensor* zero_points = GetTemporary(context, node, /*index=*/4);
        TfLiteTensor* row_sums = GetTemporary(context, node, /*index=*/5);
        // Dequantize weights time.
        // TODO(alanchiao): this dequantization initialization only needs to
        // happen once per model and should theoretically be placed in either
        // Init or Prepare. However, TFLite doesn't allocate float_weights_time
        // until the Eval function.
        // TODO(alanchiao): refactor logic out into dequantize function.
        if (!op_data->float_weights_time_initialized) {
          const float dequantization_scale = weights_time->params.scale;
          const int8_t* weights_time_ptr = GetTensorData<int8_t>(weights_time);
          float* float_weights_time_ptr =
              GetTensorData<float>(float_weights_time);
          for (int i = 0; i < NumElements(float_weights_time); ++i) {
            float_weights_time_ptr[i] =
                weights_time_ptr[i] * dequantization_scale;
          }
          op_data->float_weights_time_initialized = true;
        }

        reference_ops::EvalHybridSVDF(
            context, node, input, weights_feature, float_weights_time, bias,
            params, scratch, scaling_factors, input_quantized, state, output,
            zero_points, row_sums, &op_data->compute_row_sums);
        return kTfLiteOk;
      } else {
        auto* input_params = reinterpret_cast<TfLiteAffineQuantization*>(
            input->quantization.params);
        auto* output_params = reinterpret_cast<TfLiteAffineQuantization*>(
            output->quantization.params);
        TfLiteTensor* output_temp = GetTemporary(context, node, /*index=*/1);

        // Currently supports only ReLU.
        // TODO(jianlijianli): support other activations.
        TF_LITE_ENSURE_EQ(context, params->activation, kTfLiteActRelu);
        reference_ops::EvalIntegerSVDF(
            context, node, input, weights_feature, weights_time, bias, params,
            state, output, scratch, output_temp, op_data->effective_scale_1_a,
            op_data->effective_scale_1_b, op_data->effective_scale_2_a,
            op_data->effective_scale_2_b, input_params->zero_point->data[0],
            output_params->zero_point->data[0]);
        return kTfLiteOk;
      }
      break;
    }
    default:
      context->ReportError(context, "Type %s not currently supported.",
                           TfLiteTypeGetName(weights_feature->type));
      return kTfLiteError;
  }
}