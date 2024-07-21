TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const bool is_soft_nms = NumInputs(node) == 6;

  const TfLiteTensor* input_boxes = GetInput(context, node, kInputTensorBoxes);
  const int num_boxes = SizeOfDimension(input_boxes, 0);
  const TfLiteTensor* input_scores =
      GetInput(context, node, kInputTensorScores);
  const TfLiteTensor* input_max_output_size =
      GetInput(context, node, kInputTensorMaxOutputSize);
  const int max_output_size_value = *GetTensorData<int>(input_max_output_size);
  TF_LITE_ENSURE(context, (max_output_size_value >= 0));
  const bool is_max_output_size_const = IsConstantTensor(input_max_output_size);
  const TfLiteTensor* input_iou_threshold =
      GetInput(context, node, kInputTensorIouThreshold);
  const float iou_threshold = *GetTensorData<float>(input_iou_threshold);
  const TfLiteTensor* input_score_threshold =
      GetInput(context, node, kInputTensorScoreThreshold);
  const float score_threshold = *GetTensorData<float>(input_score_threshold);

  TfLiteTensor* output_selected_indices = nullptr;
  TfLiteTensor* output_selected_scores = nullptr;
  TfLiteTensor* output_num_selected_indices = nullptr;

  if (is_soft_nms) {
    const TfLiteTensor* input_sigma =
        GetInput(context, node, kInputTensorSigma);
    const float soft_nms_sigma = *GetTensorData<float>(input_sigma);
    if (soft_nms_sigma < 0) {
      context->ReportError(context, "Invalid sigma value for soft NMS: %f",
                           soft_nms_sigma);
      return kTfLiteError;
    }

    output_selected_indices =
        GetOutput(context, node, kSoftNMSOutputTensorSelectedIndices);
    output_selected_scores =
        GetOutput(context, node, kSoftNMSOutputTensorSelectedScores);
    output_num_selected_indices =
        GetOutput(context, node, kSoftNMSOutputTensorNumSelectedIndices);
    if (!is_max_output_size_const) {
      SetTensorSizes(context, output_selected_indices, {max_output_size_value});
      SetTensorSizes(context, output_selected_scores, {max_output_size_value});
    }
    reference_ops::NonMaxSuppression(
        input_boxes->data.f, num_boxes, input_scores->data.f,
        max_output_size_value, iou_threshold, score_threshold, soft_nms_sigma,
        output_selected_indices->data.i32, output_selected_scores->data.f,
        output_num_selected_indices->data.i32);
    ResetUnusedElementsToZeroes(
        max_output_size_value, *output_num_selected_indices->data.i32,
        output_selected_indices->data.i32, output_selected_scores->data.f);
  } else {
    output_selected_indices =
        GetOutput(context, node, kNMSOutputTensorSelectedIndices);
    output_num_selected_indices =
        GetOutput(context, node, kNMSOutputTensorNumSelectedIndices);
    if (!is_max_output_size_const) {
      SetTensorSizes(context, output_selected_indices, {max_output_size_value});
    }
    reference_ops::NonMaxSuppression(
        input_boxes->data.f, num_boxes, input_scores->data.f,
        max_output_size_value, iou_threshold, score_threshold, /**sigma=**/ 0.0,
        output_selected_indices->data.i32, /**selected_scores=**/ nullptr,
        output_num_selected_indices->data.i32);
    ResetUnusedElementsToZeroes(max_output_size_value,
                                *output_num_selected_indices->data.i32,
                                output_selected_indices->data.i32, nullptr);
  }

  return kTfLiteOk;
}