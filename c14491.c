TfLiteStatus EluPrepare(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, 0);
  TfLiteTensor* output = GetOutput(context, node, 0);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  // Use LUT to handle quantized elu path.
  if (input->type == kTfLiteInt8) {
    PopulateLookupTable<int8_t>(data, input, output, [](float value) {
      return value < 0.0 ? std::exp(value) - 1.0f : value;
    });
  }
  return GenericPrepare(context, node);
}