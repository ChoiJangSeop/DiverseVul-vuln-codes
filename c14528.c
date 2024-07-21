TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteSequenceRNNParams*>(node->builtin_data);
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* input_weights = GetInput(context, node, kWeightsTensor);
  const TfLiteTensor* recurrent_weights =
      GetInput(context, node, kRecurrentWeightsTensor);
  const TfLiteTensor* bias = GetInput(context, node, kBiasTensor);
  // The hidden_state is a variable input tensor that can be modified.
  TfLiteTensor* hidden_state =
      const_cast<TfLiteTensor*>(GetInput(context, node, kHiddenStateTensor));
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (input_weights->type) {
    case kTfLiteFloat32:
      return EvalFloat(input, input_weights, recurrent_weights, bias, params,
                       hidden_state, output);
    case kTfLiteUInt8:
    case kTfLiteInt8: {
      // TODO(mirkov): implement eval with quantized inputs as well.
      auto* op_data = reinterpret_cast<OpData*>(node->user_data);
      TfLiteTensor* input_quantized = GetTemporary(context, node, 0);
      TfLiteTensor* hidden_state_quantized = GetTemporary(context, node, 1);
      TfLiteTensor* scaling_factors = GetTemporary(context, node, 2);
      TfLiteTensor* accum_scratch = GetTemporary(context, node, 3);
      TfLiteTensor* zero_points = GetTemporary(context, node, 4);
      TfLiteTensor* row_sums = GetTemporary(context, node, 5);
      return EvalHybrid(input, input_weights, recurrent_weights, bias, params,
                        input_quantized, hidden_state_quantized,
                        scaling_factors, hidden_state, output, zero_points,
                        accum_scratch, row_sums, &op_data->compute_row_sums);
    }
    default:
      TF_LITE_KERNEL_LOG(context, "Type %d not currently supported.",
                         TfLiteTypeGetName(input_weights->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}