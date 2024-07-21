TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TfLiteTensor* output_values = GetOutput(context, node, kOutputValues);
  TfLiteTensor* output_indexes = GetOutput(context, node, kOutputIndexes);
  if (IsDynamicTensor(output_values)) {
    TF_LITE_ENSURE_OK(context, ResizeOutput(context, node));
  }
  const TfLiteTensor* top_k = GetInput(context, node, kInputTopK);
  const int32 k = top_k->data.i32[0];
  // The tensor can have more than 2 dimensions or even be a vector, the code
  // anyway calls the internal dimension as row;
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const int32 row_size = input->dims->data[input->dims->size - 1];
  int32 num_rows = 1;
  for (int i = 0; i < input->dims->size - 1; ++i) {
    num_rows *= input->dims->data[i];
  }
  switch (output_values->type) {
    case kTfLiteFloat32:
      TopK(row_size, num_rows, GetTensorData<float>(input), k,
           output_indexes->data.i32, GetTensorData<float>(output_values));
      break;
    case kTfLiteUInt8:
      TopK(row_size, num_rows, input->data.uint8, k, output_indexes->data.i32,
           output_values->data.uint8);
      break;
    case kTfLiteInt8:
      TopK(row_size, num_rows, input->data.int8, k, output_indexes->data.i32,
           output_values->data.int8);
      break;
    case kTfLiteInt32:
      TopK(row_size, num_rows, input->data.i32, k, output_indexes->data.i32,
           output_values->data.i32);
      break;
    case kTfLiteInt64:
      TopK(row_size, num_rows, input->data.i64, k, output_indexes->data.i32,
           output_values->data.i64);
      break;
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s is currently not supported by TopK.",
                         TfLiteTypeGetName(output_values->type));
      return kTfLiteError;
  }

  return kTfLiteOk;
}