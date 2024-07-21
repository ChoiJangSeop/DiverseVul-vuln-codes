TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  Subgraph* subgraph = reinterpret_cast<Subgraph*>(context->impl_);

  const TfLiteTensor* input_resource_id_tensor =
      GetInput(context, node, kInputVariableId);
  const TfLiteTensor* input_value_tensor = GetInput(context, node, kInputValue);

  int resource_id = input_resource_id_tensor->data.i32[0];
  auto& resources = subgraph->resources();
  resource::CreateResourceVariableIfNotAvailable(&resources, resource_id);
  auto* variable = resource::GetResourceVariable(&resources, resource_id);
  TF_LITE_ENSURE(context, variable != nullptr);
  variable->AssignFrom(input_value_tensor);

  return kTfLiteOk;
}