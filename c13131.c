void AverageEvalQuantizedUint8(TfLiteContext* context, TfLiteNode* node,
                               TfLitePoolParams* params, OpData* data,
                               const TfLiteTensor* input,
                               TfLiteTensor* output) {
  int32_t activation_min;
  int32_t activation_max;
  (void)CalculateActivationRangeQuantized(context, params->activation, output,
                                          &activation_min, &activation_max);
#define TF_LITE_AVERAGE_POOL(type)                                         \
  tflite::PoolParams op_params;                                            \
  op_params.stride_height = params->stride_height;                         \
  op_params.stride_width = params->stride_width;                           \
  op_params.filter_height = params->filter_height;                         \
  op_params.filter_width = params->filter_width;                           \
  op_params.padding_values.height = data->padding.height;                  \
  op_params.padding_values.width = data->padding.width;                    \
  op_params.quantized_activation_min = activation_min;                     \
  op_params.quantized_activation_max = activation_max;                     \
  type::AveragePool(op_params, GetTensorShape(input),                      \
                    GetTensorData<uint8_t>(input), GetTensorShape(output), \
                    GetTensorData<uint8_t>(output))
  if (kernel_type == kReference) {
    TF_LITE_AVERAGE_POOL(reference_ops);
  } else {
    TF_LITE_AVERAGE_POOL(optimized_ops);
  }
#undef TF_LITE_AVERAGE_POOL
}