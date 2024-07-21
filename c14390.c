TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  auto* op_data = static_cast<OpData*>(node->user_data);
  // Inputs: box_encodings, scores, anchors
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 3);
  const TfLiteTensor* input_box_encodings =
      GetInput(context, node, kInputTensorBoxEncodings);
  const TfLiteTensor* input_class_predictions =
      GetInput(context, node, kInputTensorClassPredictions);
  const TfLiteTensor* input_anchors =
      GetInput(context, node, kInputTensorAnchors);
  TF_LITE_ENSURE_EQ(context, NumDimensions(input_box_encodings), 3);
  TF_LITE_ENSURE_EQ(context, NumDimensions(input_class_predictions), 3);
  TF_LITE_ENSURE_EQ(context, NumDimensions(input_anchors), 2);
  // number of detected boxes
  const int num_detected_boxes =
      op_data->max_detections * op_data->max_classes_per_detection;

  // Outputs: detection_boxes, detection_scores, detection_classes,
  // num_detections
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 4);
  // Output Tensor detection_boxes: size is set to (1, num_detected_boxes, 4)
  TfLiteTensor* detection_boxes =
      GetOutput(context, node, kOutputTensorDetectionBoxes);
  detection_boxes->type = kTfLiteFloat32;
  SetTensorSizes(context, detection_boxes,
                 {kBatchSize, num_detected_boxes, kNumCoordBox});

  // Output Tensor detection_classes: size is set to (1, num_detected_boxes)
  TfLiteTensor* detection_classes =
      GetOutput(context, node, kOutputTensorDetectionClasses);
  detection_classes->type = kTfLiteFloat32;
  SetTensorSizes(context, detection_classes, {kBatchSize, num_detected_boxes});

  // Output Tensor detection_scores: size is set to (1, num_detected_boxes)
  TfLiteTensor* detection_scores =
      GetOutput(context, node, kOutputTensorDetectionScores);
  detection_scores->type = kTfLiteFloat32;
  SetTensorSizes(context, detection_scores, {kBatchSize, num_detected_boxes});

  // Output Tensor num_detections: size is set to 1
  TfLiteTensor* num_detections =
      GetOutput(context, node, kOutputTensorNumDetections);
  num_detections->type = kTfLiteFloat32;
  // TODO (chowdhery): Make it a scalar when available
  SetTensorSizes(context, num_detections, {1});

  // Temporary tensors
  TfLiteIntArrayFree(node->temporaries);
  node->temporaries = TfLiteIntArrayCreate(3);
  node->temporaries->data[0] = op_data->decoded_boxes_index;
  node->temporaries->data[1] = op_data->scores_index;
  node->temporaries->data[2] = op_data->active_candidate_index;

  // decoded_boxes
  TfLiteTensor* decoded_boxes = &context->tensors[op_data->decoded_boxes_index];
  decoded_boxes->type = kTfLiteFloat32;
  decoded_boxes->allocation_type = kTfLiteArenaRw;
  SetTensorSizes(context, decoded_boxes,
                 {input_box_encodings->dims->data[1], kNumCoordBox});

  // scores
  TfLiteTensor* scores = &context->tensors[op_data->scores_index];
  scores->type = kTfLiteFloat32;
  scores->allocation_type = kTfLiteArenaRw;
  SetTensorSizes(context, scores,
                 {input_class_predictions->dims->data[1],
                  input_class_predictions->dims->data[2]});

  // active_candidate
  TfLiteTensor* active_candidate =
      &context->tensors[op_data->active_candidate_index];
  active_candidate->type = kTfLiteUInt8;
  active_candidate->allocation_type = kTfLiteArenaRw;
  SetTensorSizes(context, active_candidate,
                 {input_box_encodings->dims->data[1]});

  return kTfLiteOk;
}