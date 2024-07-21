TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteFullyConnectedParams*>(node->builtin_data);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* filter = GetInput(context, node, kWeightsTensor);
  const TfLiteTensor* bias =
      (node->inputs->size == 3)
          ? GetOptionalInputTensor(context, node, kBiasTensor)
          : nullptr;
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (filter->type) {
    case kTfLiteFloat32:
      return EvalFloat<kernel_type>(context, node, params, data, input, filter,
                                    bias, output);
    case kTfLiteUInt8:
      if (params->weights_format ==
          kTfLiteFullyConnectedWeightsFormatShuffled4x16Int8) {
        TfLiteTensor* shuffled_input_workspace =
            GetOutput(context, node, kShuffledInputWorkspaceTensor);
        return EvalShuffledQuantized<kernel_type>(context, node, params, data,
                                                  input, filter, bias, output,
                                                  shuffled_input_workspace);
      } else if (params->weights_format ==
                 kTfLiteFullyConnectedWeightsFormatDefault) {
        return EvalQuantized<kernel_type>(context, node, params, data, input,
                                          filter, bias, output);
      } else {
        context->ReportError(context,
                             "Unhandled fully-connected weights format");
        return kTfLiteError;
      }
    case kTfLiteInt8:
      if (params->weights_format == kTfLiteFullyConnectedWeightsFormatDefault) {
        return EvalQuantized<kernel_type>(context, node, params, data, input,
                                          filter, bias, output);
      } else {
        context->ReportError(context,
                             "Unhandled fully-connected weights format");
        return kTfLiteError;
      }
    default:
      context->ReportError(context,
                           "Filter data type %s currently not supported.",
                           TfLiteTypeGetName(filter->type));
      return kTfLiteError;
  }
  return kTfLiteOk;
}