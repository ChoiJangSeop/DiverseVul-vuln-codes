TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  Subgraph* subgraph = reinterpret_cast<Subgraph*>(context->impl_);

  const TfLiteTensor* input_resource_id_tensor =
      GetInput(context, node, kInputVariableId);
  int resource_id = input_resource_id_tensor->data.i32[0];
  auto& resources = subgraph->resources();
  auto* variable = resource::GetResourceVariable(&resources, resource_id);
  TF_LITE_ENSURE(context, variable != nullptr);

  TfLiteTensor* variable_tensor = variable->GetTensor();
  TfLiteTensor* output = GetOutput(context, node, kOutputValue);

  TF_LITE_ENSURE_TYPES_EQ(context, variable_tensor->type, output->type);
  TF_LITE_ENSURE_OK(
      context, context->ResizeTensor(
                   context, output, TfLiteIntArrayCopy(variable_tensor->dims)));
  memcpy(output->data.raw, variable_tensor->data.raw, output->bytes);

  return kTfLiteOk;
}