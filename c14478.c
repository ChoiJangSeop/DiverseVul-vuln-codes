TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  const OpData* op_data = reinterpret_cast<OpData*>(node->user_data);

  TF_LITE_ENSURE(context, node->inputs->size > 0);

  // The first input is the condition.
  const TfLiteTensor* cond = GetInput(context, node, 0);
  // Currently only bool is supported.
  // TODO(ycling): Support other types since TensorFlow also support
  // non-bool types as condition.
  TF_LITE_ENSURE_EQ(context, cond->type, kTfLiteBool);
  TF_LITE_ENSURE_EQ(context, NumElements(cond), 1);

  // The first input of the node is the condition. The rest of inputs are
  // passed to the branch subgraphs. Therefore, the number of subgraph inputs
  // will be the number of node inputs - 1.
  int num_inputs = node->inputs->size - 1;
  int num_outputs = node->outputs->size;

  Subgraph* this_subgraph = reinterpret_cast<Subgraph*>(context->impl_);
  auto* subgraphs = this_subgraph->GetSubgraphs();
  TF_LITE_ENSURE(context, op_data->then_subgraph_index < subgraphs->size());
  TF_LITE_ENSURE(context, op_data->else_subgraph_index < subgraphs->size());

  Subgraph* then_subgraph = (*subgraphs)[op_data->then_subgraph_index].get();
  Subgraph* else_subgraph = (*subgraphs)[op_data->else_subgraph_index].get();

  for (auto* subgraph : {then_subgraph, else_subgraph}) {
    TF_LITE_ENSURE_EQ(context, num_inputs, subgraph->inputs().size());
    TF_LITE_ENSURE_EQ(context, num_outputs, subgraph->outputs().size());
  }

  bool has_dynamic_output_tensors = false;
  for (auto* subgraph : {then_subgraph, else_subgraph}) {
    for (int i = 0; i < num_inputs; ++i) {
      // The first input of the node is the condition. The indices of the inputs
      // passed to the subgraphs are offset by 1.
      const TfLiteTensor* input = GetInput(context, node, i + 1);
      std::vector<int> dims(input->dims->data,
                            input->dims->data + input->dims->size);
      subgraph->ResizeInputTensor(i, dims);
      TfLiteTensor* subgraph_input = subgraph->tensor(subgraph->inputs()[i]);
      TF_LITE_ENSURE_TYPES_EQ(context, input->type, subgraph_input->type);
    }
    // Note: The `Prepare` function is responsible to run `AllocateTensors` on
    // both subgraphs. It's intentionally not to break out of the loop when
    // finding a dynamic output tensor.
    TF_LITE_ENSURE_OK(context, subgraph->AllocateTensors());
    has_dynamic_output_tensors |= subgraph->HasDynamicTensors();
  }

  if (!has_dynamic_output_tensors) {
    for (int i = 0; i < num_outputs; ++i) {
      TfLiteTensor* then_output =
          then_subgraph->tensor(then_subgraph->outputs()[i]);
      TfLiteTensor* else_output =
          else_subgraph->tensor(else_subgraph->outputs()[i]);
      // If the 2 subgraphs have static but different output shapes, the output
      // tensors of the IF op have dynamic sizes.
      if (!TfLiteIntArrayEqual(then_output->dims, else_output->dims)) {
        has_dynamic_output_tensors = true;
        break;
      }
    }
  }

  for (int i = 0; i < num_outputs; ++i) {
    TfLiteTensor* output = GetOutput(context, node, i);
    if (has_dynamic_output_tensors) {
      SetTensorToDynamic(output);
    } else {
      // When there's no dynamic output tensors, the 2 subgraph has exactly
      // the same static sized outputs.
      TfLiteTensor* then_output =
          then_subgraph->tensor(then_subgraph->outputs()[i]);
      TfLiteIntArray* output_size = TfLiteIntArrayCopy(then_output->dims);
      TF_LITE_ENSURE_OK(context,
                        context->ResizeTensor(context, output, output_size));
    }
  }

  return kTfLiteOk;
}