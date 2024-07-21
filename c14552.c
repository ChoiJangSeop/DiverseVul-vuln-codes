TfLiteStatus AverageEval(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLitePoolParams*>(node->builtin_data);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  TfLiteTensor* output = GetOutput(context, node, 0);
  const TfLiteTensor* input = GetInput(context, node, 0);
  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32:
      AverageEvalFloat<kernel_type>(context, node, params, data, input, output);
      break;
    case kTfLiteUInt8:
      AverageEvalQuantizedUint8<kernel_type>(context, node, params, data, input,
                                             output);
      break;
    case kTfLiteInt8:
      AverageEvalQuantizedInt8<kernel_type>(context, node, params, data, input,
                                            output);
      break;
    case kTfLiteInt16:
      AverageEvalQuantizedInt16<kernel_type>(context, node, params, data, input,
                                             output);
      break;
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s not currently supported.",
                         TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}