TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteMfccParams*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input_wav = GetInput(context, node, kInputTensorWav);
  const TfLiteTensor* input_rate = GetInput(context, node, kInputTensorRate);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  TF_LITE_ENSURE_EQ(context, NumDimensions(input_wav), 3);
  TF_LITE_ENSURE_EQ(context, NumElements(input_rate), 1);

  TF_LITE_ENSURE_TYPES_EQ(context, output->type, kTfLiteFloat32);
  TF_LITE_ENSURE_TYPES_EQ(context, input_wav->type, output->type);
  TF_LITE_ENSURE_TYPES_EQ(context, input_rate->type, kTfLiteInt32);

  TfLiteIntArray* output_size = TfLiteIntArrayCreate(3);
  output_size->data[0] = input_wav->dims->data[0];
  output_size->data[1] = input_wav->dims->data[1];
  output_size->data[2] = params->dct_coefficient_count;

  return context->ResizeTensor(context, output, output_size);
}