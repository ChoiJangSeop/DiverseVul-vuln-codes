TfLiteStatus NonMaxSuppressionMultiClass(TfLiteContext* context,
                                         TfLiteNode* node, OpData* op_data) {
  // Get the input tensors
  const TfLiteTensor* input_box_encodings =
      GetInput(context, node, kInputTensorBoxEncodings);
  const TfLiteTensor* input_class_predictions =
      GetInput(context, node, kInputTensorClassPredictions);
  const int num_boxes = input_box_encodings->dims->data[1];
  const int num_classes = op_data->num_classes;
  TF_LITE_ENSURE_EQ(context, input_class_predictions->dims->data[0],
                    kBatchSize);
  TF_LITE_ENSURE_EQ(context, input_class_predictions->dims->data[1], num_boxes);
  const int num_classes_with_background =
      input_class_predictions->dims->data[2];

  TF_LITE_ENSURE(context, (num_classes_with_background - num_classes <= 1));
  TF_LITE_ENSURE(context, (num_classes_with_background >= num_classes));

  const TfLiteTensor* scores;
  switch (input_class_predictions->type) {
    case kTfLiteUInt8: {
      TfLiteTensor* temporary_scores = &context->tensors[op_data->scores_index];
      DequantizeClassPredictions(input_class_predictions, num_boxes,
                                 num_classes_with_background, temporary_scores);
      scores = temporary_scores;
    } break;
    case kTfLiteFloat32:
      scores = input_class_predictions;
      break;
    default:
      // Unsupported type.
      return kTfLiteError;
  }
  if (op_data->use_regular_non_max_suppression)
    TF_LITE_ENSURE_STATUS(NonMaxSuppressionMultiClassRegularHelper(
        context, node, op_data, GetTensorData<float>(scores)));
  else
    TF_LITE_ENSURE_STATUS(NonMaxSuppressionMultiClassFastHelper(
        context, node, op_data, GetTensorData<float>(scores)));

  return kTfLiteOk;
}