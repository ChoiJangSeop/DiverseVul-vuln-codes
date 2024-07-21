TfLiteStatus NonMaxSuppressionSingleClassHelper(
    TfLiteContext* context, TfLiteNode* node, OpData* op_data,
    const std::vector<float>& scores, std::vector<int>* selected,
    int max_detections) {
  const TfLiteTensor* input_box_encodings =
      GetInput(context, node, kInputTensorBoxEncodings);
  const TfLiteTensor* decoded_boxes =
      &context->tensors[op_data->decoded_boxes_index];
  const int num_boxes = input_box_encodings->dims->data[1];
  const float non_max_suppression_score_threshold =
      op_data->non_max_suppression_score_threshold;
  const float intersection_over_union_threshold =
      op_data->intersection_over_union_threshold;
  // Maximum detections should be positive.
  TF_LITE_ENSURE(context, (max_detections >= 0));
  // intersection_over_union_threshold should be positive
  // and should be less than 1.
  TF_LITE_ENSURE(context, (intersection_over_union_threshold > 0.0f) &&
                              (intersection_over_union_threshold <= 1.0f));
  // Validate boxes
  TF_LITE_ENSURE(context, ValidateBoxes(decoded_boxes, num_boxes));

  // threshold scores
  std::vector<int> keep_indices;
  // TODO (chowdhery): Remove the dynamic allocation and replace it
  // with temporaries, esp for std::vector<float>
  std::vector<float> keep_scores;
  SelectDetectionsAboveScoreThreshold(
      scores, non_max_suppression_score_threshold, &keep_scores, &keep_indices);

  int num_scores_kept = keep_scores.size();
  std::vector<int> sorted_indices;
  sorted_indices.resize(num_scores_kept);
  DecreasingPartialArgSort(keep_scores.data(), num_scores_kept, num_scores_kept,
                           sorted_indices.data());
  const int num_boxes_kept = num_scores_kept;
  const int output_size = std::min(num_boxes_kept, max_detections);
  selected->clear();
  TfLiteTensor* active_candidate =
      &context->tensors[op_data->active_candidate_index];
  TF_LITE_ENSURE(context, (active_candidate->dims->data[0]) == num_boxes);
  int num_active_candidate = num_boxes_kept;
  uint8_t* active_box_candidate = (active_candidate->data.uint8);
  for (int row = 0; row < num_boxes_kept; row++) {
    active_box_candidate[row] = 1;
  }

  for (int i = 0; i < num_boxes_kept; ++i) {
    if (num_active_candidate == 0 || selected->size() >= output_size) break;
    if (active_box_candidate[i] == 1) {
      selected->push_back(keep_indices[sorted_indices[i]]);
      active_box_candidate[i] = 0;
      num_active_candidate--;
    } else {
      continue;
    }
    for (int j = i + 1; j < num_boxes_kept; ++j) {
      if (active_box_candidate[j] == 1) {
        float intersection_over_union = ComputeIntersectionOverUnion(
            decoded_boxes, keep_indices[sorted_indices[i]],
            keep_indices[sorted_indices[j]]);

        if (intersection_over_union > intersection_over_union_threshold) {
          active_box_candidate[j] = 0;
          num_active_candidate--;
        }
      }
    }
  }
  return kTfLiteOk;
}