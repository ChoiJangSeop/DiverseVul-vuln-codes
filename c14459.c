TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 0);
  OpData* op_data = reinterpret_cast<OpData*>(node->user_data);

  OpContext op_context(context, node);

  TF_LITE_ENSURE(context, op_context.input->type == kTfLiteUInt8 ||
                              op_context.input->type == kTfLiteInt8 ||
                              op_context.input->type == kTfLiteInt16 ||
                              op_context.input->type == kTfLiteFloat16);
  TF_LITE_ENSURE(context, op_context.ref->type == kTfLiteFloat32);

  op_data->max_diff = op_data->tolerance * op_context.input->params.scale;
  switch (op_context.input->type) {
    case kTfLiteUInt8:
    case kTfLiteInt8:
      op_data->max_diff *= (1 << 8);
      break;
    case kTfLiteInt16:
      op_data->max_diff *= (1 << 16);
      break;
    default:
      break;
  }

  // Allocate tensor to store the dequantized inputs.
  if (op_data->cache_tensor_id == kTensorNotAllocated) {
    TF_LITE_ENSURE_OK(
        context, context->AddTensors(context, 1, &op_data->cache_tensor_id));
  }

  TfLiteIntArrayFree(node->temporaries);
  node->temporaries = TfLiteIntArrayCreate(1);
  node->temporaries->data[0] = op_data->cache_tensor_id;

  TfLiteTensor* dequantized = GetTemporary(context, node, /*index=*/0);
  dequantized->type = op_context.ref->type;
  dequantized->allocation_type = kTfLiteDynamic;

  TF_LITE_ENSURE_OK(context, context->ResizeTensor(
                                 context, dequantized,
                                 TfLiteIntArrayCopy(op_context.input->dims)));

  return kTfLiteOk;
}