TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TfLitePackParams* data =
      reinterpret_cast<TfLitePackParams*>(node->builtin_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), data->values_count);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input0 = GetInput(context, node, 0);
  const int dimension_size = NumDimensions(input0) + 1;
  if (data->axis < 0) {
    data->axis += dimension_size;
  }
  TF_LITE_ENSURE(context, NumDimensions(input0) >= data->axis);
  TF_LITE_ENSURE(context, data->axis >= 0);

  if (input0->type != kTfLiteInt32 && input0->type != kTfLiteFloat32 &&
      input0->type != kTfLiteUInt8 && input0->type != kTfLiteInt8 &&
      input0->type != kTfLiteInt16 && input0->type != kTfLiteInt64) {
    context->ReportError(context, "Type '%s' is not supported by pack.",
                         TfLiteTypeGetName(input0->type));
    return kTfLiteError;
  }
  // Make sure all inputs have the same shape and type.
  for (int i = 1; i < data->values_count; ++i) {
    const TfLiteTensor* input = GetInput(context, node, i);
    TF_LITE_ENSURE(context, HaveSameShapes(input0, input));
    TF_LITE_ENSURE_TYPES_EQ(context, input0->type, input->type);
  }

  // Resize output. rank R will become rank R + 1
  const TfLiteIntArray* input_shape = input0->dims;
  TfLiteIntArray* output_shape = TfLiteIntArrayCreate(dimension_size);
  int i = 0;
  for (int index = 0; index < dimension_size; ++index) {
    if (index == data->axis) {
      output_shape->data[index] = data->values_count;
    } else {
      output_shape->data[index] = input_shape->data[i++];
    }
  }

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE_TYPES_EQ(context, output->type, input0->type);

  // Guarantee input/output quantization params match as we do not support
  // packing quantized tensors.
  for (int i = 0; i < data->values_count; i++) {
    const TfLiteTensor* input = GetInput(context, node, i);
    TF_LITE_ENSURE_EQ(context, input->params.zero_point,
                      output->params.zero_point);
    TF_LITE_ENSURE_EQ(context, input->params.scale, output->params.scale);
  }

  return context->ResizeTensor(context, output, output_shape);
}