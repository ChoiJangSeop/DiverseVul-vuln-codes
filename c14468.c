TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  auto* params = reinterpret_cast<TfLiteShapeParams*>(node->builtin_data);
  switch (params->out_type) {
    case kTfLiteInt32:
      output->type = kTfLiteInt32;
      break;
    case kTfLiteInt64:
      output->type = kTfLiteInt64;
      break;
    default:
      context->ReportError(context, "Unknown shape output data type: %d",
                           params->out_type);
      return kTfLiteError;
  }

  // By design, the input shape is always known at the time of Prepare, even
  // if the preceding op that generates |input| is dynamic. Thus, we can
  // always compute the shape immediately, without waiting for Eval.
  SetTensorToPersistentRo(output);

  // Shape always produces a 1-dimensional output tensor, where each output
  // element is the length of the corresponding input tensor's dimension.
  TfLiteIntArray* output_size = TfLiteIntArrayCreate(1);
  output_size->data[0] = NumDimensions(input);
  TF_LITE_ENSURE_STATUS(context->ResizeTensor(context, output, output_size));

  TFLITE_DCHECK_EQ(NumDimensions(output), 1);
  TFLITE_DCHECK_EQ(SizeOfDimension(output, 0), NumDimensions(input));

  // Immediately propagate the known shape to the output tensor. This allows
  // downstream ops that rely on the value to use it during prepare.
  switch (output->type) {
    case kTfLiteInt32:
      ExtractShape(input, GetTensorData<int32_t>(output));
      break;
    case kTfLiteInt64:
      ExtractShape(input, GetTensorData<int64_t>(output));
      break;
    default:
      return kTfLiteError;
  }

  return kTfLiteOk;
}