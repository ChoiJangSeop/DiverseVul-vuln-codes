TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* seq_lengths = GetInput(context, node, kSeqLengthsTensor);
  TF_LITE_ENSURE_EQ(context, NumDimensions(seq_lengths), 1);

  if (input->type != kTfLiteInt32 && input->type != kTfLiteFloat32 &&
      input->type != kTfLiteUInt8 && input->type != kTfLiteInt16 &&
      input->type != kTfLiteInt64) {
    context->ReportError(context,
                         "Type '%s' is not supported by reverse_sequence.",
                         TfLiteTypeGetName(input->type));
    return kTfLiteError;
  }

  if (seq_lengths->type != kTfLiteInt32 && seq_lengths->type != kTfLiteInt64) {
    context->ReportError(
        context, "Seq_lengths type '%s' is not supported by reverse_sequence.",
        TfLiteTypeGetName(seq_lengths->type));
    return kTfLiteError;
  }

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TfLiteIntArray* output_shape = TfLiteIntArrayCopy(input->dims);
  TF_LITE_ENSURE_TYPES_EQ(context, output->type, input->type);

  return context->ResizeTensor(context, output, output_shape);
}