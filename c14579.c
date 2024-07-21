TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  const TfLiteTensor* input1 = GetInput(context, node, kInputTensor1);
  const TfLiteTensor* input2 = GetInput(context, node, kInputTensor2);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  switch (input1->type) {
    case kTfLiteInt32: {
      return EvalImpl<int32_t>(context, data->requires_broadcast, input1,
                               input2, output);
    }
    case kTfLiteInt64: {
      return EvalImpl<int64_t>(context, data->requires_broadcast, input1,
                               input2, output);
    }
    case kTfLiteFloat32: {
      return EvalImpl<float>(context, data->requires_broadcast, input1, input2,
                             output);
    }
    default: {
      context->ReportError(context, "Type '%s' is not supported by floor_mod.",
                           TfLiteTypeGetName(input1->type));
      return kTfLiteError;
    }
  }
}