TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteConcatenationParams*>(node->builtin_data);
  int axis = params->axis;
  TfLiteTensor* output = GetOutput(context, node, 0);
  if (axis < 0) axis += output->dims->size;

// TODO(ahentz): Creating 'all_inputs' below is not very efficient. We should
// allocate and populate these during Prepare().
// TODO(ycling): Activation function parameter is ignored. For now we dont have
// a model with a Concatenation with fused activation function.
#define TF_LITE_CONCATENATION(scalar)                                         \
  {                                                                           \
    VectorOfTensors<scalar> all_inputs(*context, *node->inputs);              \
    tflite::ConcatenationParams op_params;                                    \
    op_params.axis = axis;                                                    \
    op_params.inputs_count = node->inputs->size;                              \
    if (kernel_type == kReference) {                                          \
      reference_ops::Concatenation(op_params, all_inputs.shapes(),            \
                                   all_inputs.data(), GetTensorShape(output), \
                                   GetTensorData<scalar>(output));            \
    } else {                                                                  \
      optimized_ops::Concatenation(op_params, all_inputs.shapes(),            \
                                   all_inputs.data(), GetTensorShape(output), \
                                   GetTensorData<scalar>(output));            \
    }                                                                         \
  }

#define TF_LITE_CONCATENATION_QUANTIZED()                         \
  {                                                               \
    VectorOfQuantizedTensors all_inputs(*context, *node->inputs); \
    tflite::ConcatenationParams op_params;                        \
    op_params.axis = axis;                                        \
    op_params.input_zeropoint = all_inputs.zero_point();          \
    op_params.input_scale = all_inputs.scale();                   \
    op_params.inputs_count = node->inputs->size;                  \
    op_params.output_zeropoint = output->params.zero_point;       \
    op_params.output_scale = output->params.scale;                \
    if (kernel_type == kReference) {                              \
      reference_ops::ConcatenationWithScaling(                    \
          op_params, all_inputs.shapes(), all_inputs.data(),      \
          GetTensorShape(output), GetTensorData<uint8>(output));  \
    } else {                                                      \
      optimized_ops::ConcatenationWithScaling(                    \
          op_params, all_inputs.shapes(), all_inputs.data(),      \
          GetTensorShape(output), GetTensorData<uint8>(output));  \
    }                                                             \
  }

  switch (output->type) {  // Already know in/outtypes are same.
    case kTfLiteFloat32:
      TF_LITE_CONCATENATION(float);
      break;
    case kTfLiteInt32:
      TF_LITE_CONCATENATION(int32);
      break;
    case kTfLiteUInt8:
      TF_LITE_CONCATENATION_QUANTIZED();
      break;
    case kTfLiteInt8:
      TF_LITE_CONCATENATION(int8_t);
      break;
    case kTfLiteInt64:
      TF_LITE_CONCATENATION(int64_t);
      break;
    case kTfLiteInt16:
      TF_LITE_CONCATENATION(int16_t);
      break;
    default:
      context->ReportError(context, "Type '%s' is not supported currently.",
                           TfLiteTypeGetName(output->type));
      return kTfLiteError;
  }

#undef TF_LITE_CONCATENATION_QUANTIZED
#undef TF_LITE_CONCATENATION

  return kTfLiteOk;
}