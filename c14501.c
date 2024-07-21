TfLiteStatus EvalLogic(TfLiteContext* context, TfLiteNode* node,
                       OpContext* op_context, T init_value,
                       T reducer(const T current, const T in)) {
  int64_t num_axis = NumElements(op_context->axis);
  TfLiteTensor* temp_index = GetTemporary(context, node, /*index=*/0);
  TfLiteTensor* resolved_axis = GetTemporary(context, node, /*index=*/1);
  // Resize the output tensor if the output tensor is dynamic.
  if (IsDynamicTensor(op_context->output)) {
    TF_LITE_ENSURE_OK(context,
                      ResizeTempAxis(context, op_context, resolved_axis));
    TF_LITE_ENSURE_OK(context, ResizeOutputTensor(context, op_context));
  }
  if (op_context->input->type == kTfLiteUInt8 ||
      op_context->input->type == kTfLiteInt8) {
    TF_LITE_ENSURE_EQ(context, op_context->input->params.scale,
                      op_context->output->params.scale);
    TF_LITE_ENSURE_EQ(context, op_context->input->params.zero_point,
                      op_context->output->params.zero_point);
  }
  TF_LITE_ENSURE(
      context,
      reference_ops::ReduceGeneric<T>(
          GetTensorData<T>(op_context->input), op_context->input->dims->data,
          op_context->input->dims->size, GetTensorData<T>(op_context->output),
          op_context->output->dims->data, op_context->output->dims->size,
          GetTensorData<int>(op_context->axis), num_axis,
          op_context->params->keep_dims, GetTensorData<int>(temp_index),
          GetTensorData<int>(resolved_axis), init_value, reducer));
  return kTfLiteOk;
}