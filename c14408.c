TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  OpContext op_context(context, node);
  OpData* op_data = reinterpret_cast<OpData*>(node->user_data);
  const TfLiteTensor* lhs = GetInput(context, node, kInputLHSTensor);
  const TfLiteTensor* rhs = GetInput(context, node, kInputRHSTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  RuntimeShape orig_lhs_shape = GetTensorShape(lhs);
  RuntimeShape orig_rhs_shape = GetTensorShape(rhs);

  bool adj_y = op_context.params->adj_y;
  bool adj_x = op_context.params->adj_x;

  const TfLiteTensor* rhs_tensor = adj_y ? rhs : GetTempRhs(context, node, rhs);
  const TfLiteTensor* lhs_tensor = adj_x ? GetTempLhs(context, node, lhs) : lhs;
  if (!adj_y) {
    // TODO(b/154760341) Constant tensors should already be transposed, but
    // we transpose once if necessary for now.
    if (!(IsConstantTensor(rhs) && op_data->rhs_transposed)) {
      TransposeRowsColumns(context, rhs, GetTemporary(context, node, 1));
      op_data->rhs_transposed = true;
    }
  }
  if (adj_x) {
    TransposeRowsColumns(context, lhs, GetTemporary(context, node, 0));
  }
  RuntimeShape rhs_shape =
      adj_y ? orig_rhs_shape : SwapRowColumnDims(orig_rhs_shape);
  RuntimeShape lhs_shape =
      adj_x ? orig_lhs_shape : SwapRowColumnDims(orig_lhs_shape);

  switch (rhs->type) {
    case kTfLiteFloat32:
      // Note we pass RHS args first, LHS args second. See note above.
      if (kernel_type == kGenericOptimized) {
        optimized_ops::BatchMatMul(rhs_shape, GetTensorData<float>(rhs_tensor),
                                   lhs_shape, GetTensorData<float>(lhs_tensor),
                                   GetTensorShape(output),
                                   GetTensorData<float>(output),
                                   CpuBackendContext::GetFromContext(context));
      } else {
        reference_ops::BatchMatMul(rhs_shape, GetTensorData<float>(rhs_tensor),
                                   lhs_shape, GetTensorData<float>(lhs_tensor),
                                   GetTensorShape(output),
                                   GetTensorData<float>(output));
      }
      break;
    case kTfLiteInt8:
      EvalQuantized<kernel_type>(context, node, op_data, lhs_shape, lhs_tensor,
                                 rhs_shape, rhs_tensor, output);
      break;
    default:
      TF_LITE_KERNEL_LOG(context,
                         "Currently BatchMatMul doesn't support type: %s",
                         TfLiteTypeGetName(lhs->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}