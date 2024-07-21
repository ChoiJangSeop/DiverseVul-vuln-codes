TfLiteStatus EvalSum(TfLiteContext* context, TfLiteNode* node) {
  OpContext op_context(context, node);
  ruy::profiler::ScopeLabel label("Sum");
  const auto& input = op_context.input;
  const auto& output = op_context.output;
  const bool same_scale =
      (input->params.scale == output->params.scale &&
       input->params.zero_point == output->params.zero_point);
  const bool eight_bit_quantized =
      input->type == kTfLiteUInt8 || input->type == kTfLiteInt8;
  const bool need_rescale = (eight_bit_quantized && !same_scale);
  if (need_rescale) {
    // Rescaling 8bit reduce sum.
    int num_axis = static_cast<int>(NumElements(op_context.axis));
    TfLiteTensor* temp_index = GetTemporary(context, node, /*index=*/0);
    TfLiteTensor* resolved_axis = GetTemporary(context, node, /*index=*/1);
    TfLiteTensor* temp_sum = GetTemporary(context, node, /*index=*/2);
    // Resize the output tensor if the output tensor is dynamic.
    if (IsDynamicTensor(op_context.output)) {
      TF_LITE_ENSURE_OK(context,
                        ResizeTempAxis(context, &op_context, resolved_axis));
      TF_LITE_ENSURE_OK(context, ResizeOutputTensor(context, &op_context));
      TF_LITE_ENSURE_OK(context, ResizeTempSum(context, &op_context, temp_sum));
    }
    if (input->type == kTfLiteUInt8) {
      TF_LITE_ENSURE(
          context,
          reference_ops::QuantizedMeanOrSum<>(
              GetTensorData<uint8_t>(op_context.input),
              op_context.input->params.zero_point,
              op_context.input->params.scale, op_context.input->dims->data,
              op_context.input->dims->size,
              GetTensorData<uint8_t>(op_context.output),
              op_context.output->params.zero_point,
              op_context.output->params.scale, op_context.output->dims->data,
              op_context.output->dims->size,
              GetTensorData<int>(op_context.axis), num_axis,
              op_context.params->keep_dims, GetTensorData<int>(temp_index),
              GetTensorData<int>(resolved_axis), GetTensorData<int32>(temp_sum),
              /*compute_sum=*/true));
    }
    if (input->type == kTfLiteInt8) {
      TF_LITE_ENSURE(
          context,
          reference_ops::QuantizedMeanOrSum<>(
              GetTensorData<int8_t>(op_context.input),
              op_context.input->params.zero_point,
              op_context.input->params.scale, op_context.input->dims->data,
              op_context.input->dims->size,
              GetTensorData<int8_t>(op_context.output),
              op_context.output->params.zero_point,
              op_context.output->params.scale, op_context.output->dims->data,
              op_context.output->dims->size,
              GetTensorData<int>(op_context.axis), num_axis,
              op_context.params->keep_dims, GetTensorData<int>(temp_index),
              GetTensorData<int>(resolved_axis), GetTensorData<int32>(temp_sum),
              /*compute_sum=*/true));
    }
  } else {
    return EvalGeneric<kReference, kSum>(context, node);
  }

  return kTfLiteOk;
}