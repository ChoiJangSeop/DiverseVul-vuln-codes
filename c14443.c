TfLiteStatus NonMaxSuppressionMultiClassRegularHelper(TfLiteContext* context,
                                                      TfLiteNode* node,
                                                      OpData* op_data,
                                                      const float* scores) {
  const TfLiteTensor* input_box_encodings =
      GetInput(context, node, kInputTensorBoxEncodings);
  const TfLiteTensor* input_class_predictions =
      GetInput(context, node, kInputTensorClassPredictions);
  const TfLiteTensor* decoded_boxes =
      &context->tensors[op_data->decoded_boxes_index];

  TfLiteTensor* detection_boxes =
      GetOutput(context, node, kOutputTensorDetectionBoxes);
  TfLiteTensor* detection_classes =
      GetOutput(context, node, kOutputTensorDetectionClasses);
  TfLiteTensor* detection_scores =
      GetOutput(context, node, kOutputTensorDetectionScores);
  TfLiteTensor* num_detections =
      GetOutput(context, node, kOutputTensorNumDetections);

  const int num_boxes = input_box_encodings->dims->data[1];
  const int num_classes = op_data->num_classes;
  const int num_detections_per_class = op_data->detections_per_class;
  const int max_detections = op_data->max_detections;
  const int num_classes_with_background =
      input_class_predictions->dims->data[2];
  // The row index offset is 1 if background class is included and 0 otherwise.
  int label_offset = num_classes_with_background - num_classes;
  TF_LITE_ENSURE(context, num_detections_per_class > 0);

  // For each class, perform non-max suppression.
  std::vector<float> class_scores(num_boxes);

  std::vector<int> box_indices_after_regular_non_max_suppression(
      num_boxes + max_detections);
  std::vector<float> scores_after_regular_non_max_suppression(num_boxes +
                                                              max_detections);

  int size_of_sorted_indices = 0;
  std::vector<int> sorted_indices;
  sorted_indices.resize(num_boxes + max_detections);
  std::vector<float> sorted_values;
  sorted_values.resize(max_detections);

  for (int col = 0; col < num_classes; col++) {
    for (int row = 0; row < num_boxes; row++) {
      // Get scores of boxes corresponding to all anchors for single class
      class_scores[row] =
          *(scores + row * num_classes_with_background + col + label_offset);
    }
    // Perform non-maximal suppression on single class
    std::vector<int> selected;
    TF_LITE_ENSURE_STATUS(NonMaxSuppressionSingleClassHelper(
        context, node, op_data, class_scores, &selected,
        num_detections_per_class));
    // Add selected indices from non-max suppression of boxes in this class
    int output_index = size_of_sorted_indices;
    for (const auto& selected_index : selected) {
      box_indices_after_regular_non_max_suppression[output_index] =
          (selected_index * num_classes_with_background + col + label_offset);
      scores_after_regular_non_max_suppression[output_index] =
          class_scores[selected_index];
      output_index++;
    }
    // Sort the max scores among the selected indices
    // Get the indices for top scores
    int num_indices_to_sort = std::min(output_index, max_detections);
    DecreasingPartialArgSort(scores_after_regular_non_max_suppression.data(),
                             output_index, num_indices_to_sort,
                             sorted_indices.data());

    // Copy values to temporary vectors
    for (int row = 0; row < num_indices_to_sort; row++) {
      int temp = sorted_indices[row];
      sorted_indices[row] = box_indices_after_regular_non_max_suppression[temp];
      sorted_values[row] = scores_after_regular_non_max_suppression[temp];
    }
    // Copy scores and indices from temporary vectors
    for (int row = 0; row < num_indices_to_sort; row++) {
      box_indices_after_regular_non_max_suppression[row] = sorted_indices[row];
      scores_after_regular_non_max_suppression[row] = sorted_values[row];
    }
    size_of_sorted_indices = num_indices_to_sort;
  }

  // Allocate output tensors
  for (int output_box_index = 0; output_box_index < max_detections;
       output_box_index++) {
    if (output_box_index < size_of_sorted_indices) {
      const int anchor_index = floor(
          box_indices_after_regular_non_max_suppression[output_box_index] /
          num_classes_with_background);
      const int class_index =
          box_indices_after_regular_non_max_suppression[output_box_index] -
          anchor_index * num_classes_with_background - label_offset;
      const float selected_score =
          scores_after_regular_non_max_suppression[output_box_index];
      // detection_boxes
      ReInterpretTensor<BoxCornerEncoding*>(detection_boxes)[output_box_index] =
          ReInterpretTensor<const BoxCornerEncoding*>(
              decoded_boxes)[anchor_index];
      // detection_classes
      GetTensorData<float>(detection_classes)[output_box_index] = class_index;
      // detection_scores
      GetTensorData<float>(detection_scores)[output_box_index] = selected_score;
    } else {
      ReInterpretTensor<BoxCornerEncoding*>(
          detection_boxes)[output_box_index] = {0.0f, 0.0f, 0.0f, 0.0f};
      // detection_classes
      GetTensorData<float>(detection_classes)[output_box_index] = 0.0f;
      // detection_scores
      GetTensorData<float>(detection_scores)[output_box_index] = 0.0f;
    }
  }
  GetTensorData<float>(num_detections)[0] = size_of_sorted_indices;
  box_indices_after_regular_non_max_suppression.clear();
  scores_after_regular_non_max_suppression.clear();
  return kTfLiteOk;
}