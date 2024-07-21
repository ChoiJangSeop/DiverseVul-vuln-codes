TfLiteStatus EvalMean(TfLiteContext* context, TfLiteNode* node) {
  OpContext op_context(context, node);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

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

  if (kernel_type == kGenericOptimized) {
    // Use optimized ops if available.
    switch (op_context.input->type) {
      case kTfLiteInt8: {
        tflite::MeanParams op_params;
        op_params.axis_count = num_axis;
        ResolveAxis(GetTensorData<int>(op_context.axis), num_axis, &op_params);
        const TfLiteTensor* input = op_context.input;
        if (op_context.params->keep_dims && NumDimensions(input) == 4 &&
            op_params.axis_count == 2 &&
            ((op_params.axis[0] == 1 && op_params.axis[1] == 2) ||
             (op_params.axis[0] == 2 && op_params.axis[1] == 1))) {
          optimized_integer_ops::Mean(
              op_params, GetTensorShape(input), GetTensorData<int8_t>(input),
              input->params.zero_point, input->params.scale,
              GetTensorShape(op_context.output),
              GetTensorData<int8_t>(op_context.output),
              op_context.output->params.zero_point,
              op_context.output->params.scale,
              CpuBackendContext::GetFromContext(context));
          return kTfLiteOk;
        }
      } break;
      case kTfLiteUInt8: {
        tflite::MeanParams op_params;
        op_params.axis_count = num_axis;
        ResolveAxis(GetTensorData<int>(op_context.axis), num_axis, &op_params);
        const TfLiteTensor* input = op_context.input;
        if (op_context.params->keep_dims && NumDimensions(input) == 4 &&
            op_params.axis_count == 2 &&
            ((op_params.axis[0] == 1 && op_params.axis[1] == 2) ||
             (op_params.axis[0] == 2 && op_params.axis[1] == 1))) {
          optimized_ops::Mean(op_params, GetTensorShape(input),
                              GetTensorData<uint8_t>(input),
                              input->params.zero_point, input->params.scale,
                              GetTensorShape(op_context.output),
                              GetTensorData<uint8_t>(op_context.output),
                              op_context.output->params.zero_point,
                              op_context.output->params.scale,
                              CpuBackendContext::GetFromContext(context));
          return kTfLiteOk;
        }
      } break;
      default:
        break;
    }
  }

  // From here, it uses the reference implementations.
  // TODO(b/139102329): Clean up the function signatures to merge the variations
  // and handle the specialized cases in the combined reference implementations
  // per each op.
  switch (op_context.input->type) {
    case kTfLiteFloat32: {
      tflite::MeanParams op_params;
      op_params.axis_count = num_axis;
      ResolveAxis(GetTensorData<int>(op_context.axis), num_axis, &op_params);
      const TfLiteTensor* input = op_context.input;
      // TODO(b/139102329): Handle the below special case in the combined
      // reference method.
      // Defer to specialized implementation for 4D Mean across axes 1 & 2.
      if (op_context.params->keep_dims && NumDimensions(input) == 4 &&
          op_params.axis_count == 2 &&
          ((op_params.axis[0] == 1 && op_params.axis[1] == 2) ||
           (op_params.axis[0] == 2 && op_params.axis[1] == 1))) {
        reference_ops::Mean(op_params, GetTensorShape(input),
                            GetTensorData<float>(input),
                            GetTensorShape(op_context.output),
                            GetTensorData<float>(op_context.output));
      } else {
        TF_LITE_ENSURE(
            context,
            optimized_ops::MeanGeneral(
                GetTensorData<float>(op_context.input),
                op_context.input->dims->data, op_context.input->dims->size,
                GetTensorData<float>(op_context.output),
                op_context.output->dims->data, op_context.output->dims->size,
                GetTensorData<int>(op_context.axis), num_axis,
                op_context.params->keep_dims, GetTensorData<int>(temp_index),
                GetTensorData<int>(resolved_axis),
                GetTensorData<float>(temp_sum)));
      }
    } break;
    case kTfLiteInt32:
      TF_LITE_ENSURE(
          context,
          reference_ops::Mean(
              GetTensorData<int>(op_context.input),
              op_context.input->dims->data, op_context.input->dims->size,
              GetTensorData<int>(op_context.output),
              op_context.output->dims->data, op_context.output->dims->size,
              GetTensorData<int>(op_context.axis), num_axis,
              op_context.params->keep_dims, GetTensorData<int>(temp_index),
              GetTensorData<int>(resolved_axis),
              GetTensorData<int64_t>(temp_sum)));
      break;
    case kTfLiteInt64:
      TF_LITE_ENSURE(
          context,
          reference_ops::Mean(
              GetTensorData<int64_t>(op_context.input),
              op_context.input->dims->data, op_context.input->dims->size,
              GetTensorData<int64_t>(op_context.output),
              op_context.output->dims->data, op_context.output->dims->size,
              GetTensorData<int>(op_context.axis), num_axis,
              op_context.params->keep_dims, GetTensorData<int>(temp_index),
              GetTensorData<int>(resolved_axis),
              GetTensorData<int64_t>(temp_sum)));
      break;
    case kTfLiteInt8: {
      TF_LITE_ENSURE_OK(context, EvalMeanReferenceOps<int8_t>(
                                     context, op_context, num_axis, data,
                                     temp_index, resolved_axis, temp_sum));
    } break;
    case kTfLiteInt16: {
      TF_LITE_ENSURE_OK(context, EvalMeanReferenceOps<int16_t>(
                                     context, op_context, num_axis, data,
                                     temp_index, resolved_axis, temp_sum));
    } break;
    case kTfLiteUInt8: {
      TF_LITE_ENSURE_OK(context, EvalMeanReferenceOps<uint8_t>(
                                     context, op_context, num_axis, data,
                                     temp_index, resolved_axis, temp_sum));
    } break;
    default:
      return kTfLiteError;
  }
  return kTfLiteOk;
}