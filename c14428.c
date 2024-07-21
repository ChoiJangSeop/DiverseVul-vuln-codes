TfLiteStatus UseDynamicOutputTensors(TfLiteContext* context, TfLiteNode* node) {
  for (int i = 0; i < NumOutputs(node); ++i) {
    SetTensorToDynamic(GetOutput(context, node, i));
  }
  return kTfLiteOk;
}