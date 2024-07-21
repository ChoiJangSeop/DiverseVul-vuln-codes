TfLiteStatus ReverseSequenceImpl(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* seq_lengths_tensor =
      GetInput(context, node, kSeqLengthsTensor);
  const TS* seq_lengths = GetTensorData<TS>(seq_lengths_tensor);

  auto* params =
      reinterpret_cast<TfLiteReverseSequenceParams*>(node->builtin_data);
  int seq_dim = params->seq_dim;
  int batch_dim = params->batch_dim;

  TF_LITE_ENSURE(context, seq_dim >= 0);
  TF_LITE_ENSURE(context, batch_dim >= 0);
  TF_LITE_ENSURE(context, seq_dim != batch_dim);
  TF_LITE_ENSURE(context, seq_dim < NumDimensions(input));
  TF_LITE_ENSURE(context, batch_dim < NumDimensions(input));
  TF_LITE_ENSURE_EQ(context, SizeOfDimension(seq_lengths_tensor, 0),
                    SizeOfDimension(input, batch_dim));
  for (int i = 0; i < NumDimensions(seq_lengths_tensor); ++i) {
    TF_LITE_ENSURE(context, seq_lengths[i] <= SizeOfDimension(input, seq_dim));
  }

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  reference_ops::ReverseSequence<T, TS>(
      seq_lengths, seq_dim, batch_dim, GetTensorShape(input),
      GetTensorData<T>(input), GetTensorShape(output),
      GetTensorData<T>(output));

  return kTfLiteOk;
}