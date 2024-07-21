TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const OpData* op_data = reinterpret_cast<OpData*>(node->user_data);

  const TfLiteTensor* cond = GetInput(context, node, 0);
  bool cond_value = cond->data.b[0];

  Subgraph* this_subgraph = reinterpret_cast<Subgraph*>(context->impl_);
  auto* subgraphs = this_subgraph->GetSubgraphs();

  // Currently we copy the input / output between the subgraphs. This isn't
  // optimized yet.
  // TODO(b/120234921): Optimize and avoid copying tensors between subgraphs.
  int active_branch_subgraph_index =
      cond_value ? op_data->then_subgraph_index : op_data->else_subgraph_index;
  Subgraph& active_branch_subgraph =
      *(*subgraphs)[active_branch_subgraph_index];
  for (int i = 0; i < active_branch_subgraph.inputs().size(); ++i) {
    const TfLiteTensor* input = GetInput(context, node, i + 1);
    TfLiteTensor* subgraph_input =
        active_branch_subgraph.tensor(active_branch_subgraph.inputs()[i]);
    TF_LITE_ENSURE_EQ(context, input->bytes, subgraph_input->bytes);
    memcpy(subgraph_input->data.raw, input->data.raw, input->bytes);
  }

  // Note: It's guaranteed that the subgraphs' `AllocateTensors` are called
  // in `Prepare`, so we don't need to do it here again.
  TF_LITE_ENSURE_OK(context, active_branch_subgraph.Invoke());

  for (int tensor_index : active_branch_subgraph.outputs()) {
    active_branch_subgraph.EnsureTensorDataIsReadable(tensor_index);
  }

  bool has_dynamic_output_tensors = false;
  for (int i = 0; i < node->outputs->size; ++i) {
    TfLiteTensor* output = GetOutput(context, node, i);
    if (IsDynamicTensor(output)) {
      has_dynamic_output_tensors = true;
      break;
    }
  }

  if (has_dynamic_output_tensors) {
    for (int i = 0; i < node->outputs->size; ++i) {
      TfLiteTensor* output = GetOutput(context, node, i);
      TfLiteTensor* subgraph_output =
          active_branch_subgraph.tensor(active_branch_subgraph.outputs()[i]);
      TfLiteIntArray* output_size = TfLiteIntArrayCopy(subgraph_output->dims);
      TF_LITE_ENSURE_OK(context,
                        context->ResizeTensor(context, output, output_size));
    }
  }

  for (int i = 0; i < active_branch_subgraph.outputs().size(); ++i) {
    const TfLiteTensor* subgraph_output =
        active_branch_subgraph.tensor(active_branch_subgraph.outputs()[i]);
    TfLiteTensor* output = GetOutput(context, node, i);
    TF_LITE_ENSURE_EQ(context, output->bytes, subgraph_output->bytes);
    memcpy(output->data.raw, subgraph_output->data.raw, output->bytes);
  }
  return kTfLiteOk;
}