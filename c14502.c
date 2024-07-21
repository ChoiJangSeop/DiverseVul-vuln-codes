TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  const int num_inputs = NumInputs(node);
  const bool is_soft_nms = num_inputs == 6;
  if (num_inputs != 5 && num_inputs != 6) {
    context->ReportError(context, "Found NMS op with invalid num inputs: %d",
                         NumInputs(node));
    return kTfLiteError;
  }

  // Boxes & Scores.
  const TfLiteTensor* input_boxes = GetInput(context, node, kInputTensorBoxes);
  TF_LITE_ENSURE_EQ(context, input_boxes->type, kTfLiteFloat32);
  TF_LITE_ENSURE_EQ(context, NumDimensions(input_boxes), 2);
  TF_LITE_ENSURE_EQ(context, SizeOfDimension(input_boxes, 1), 4);
  const int num_boxes = SizeOfDimension(input_boxes, 0);
  const TfLiteTensor* input_scores =
      GetInput(context, node, kInputTensorScores);
  TF_LITE_ENSURE_EQ(context, input_scores->type, kTfLiteFloat32);
  TF_LITE_ENSURE_EQ(context, NumDimensions(input_scores), 1);
  TF_LITE_ENSURE_EQ(context, num_boxes, SizeOfDimension(input_scores, 0));

  // Max output size.
  const TfLiteTensor* input_max_output_size =
      GetInput(context, node, kInputTensorMaxOutputSize);
  TF_LITE_ENSURE_EQ(context, input_max_output_size->type, kTfLiteInt32);
  TF_LITE_ENSURE_EQ(context, NumDimensions(input_max_output_size), 0);
  const bool is_max_output_size_const = IsConstantTensor(input_max_output_size);
  int max_output_size_value = 0;
  if (is_max_output_size_const) {
    max_output_size_value = *GetTensorData<int>(input_max_output_size);
    TF_LITE_ENSURE(context, (max_output_size_value >= 0));
  }

  // IoU & Score thresholds.
  const TfLiteTensor* input_iou_threshold =
      GetInput(context, node, kInputTensorIouThreshold);
  TF_LITE_ENSURE_EQ(context, input_iou_threshold->type, kTfLiteFloat32);
  TF_LITE_ENSURE_EQ(context, NumDimensions(input_iou_threshold), 0);
  const TfLiteTensor* input_score_threshold =
      GetInput(context, node, kInputTensorScoreThreshold);
  TF_LITE_ENSURE_EQ(context, input_iou_threshold->type, kTfLiteFloat32);
  TF_LITE_ENSURE_EQ(context, NumDimensions(input_score_threshold), 0);

  if (is_soft_nms) {
    const TfLiteTensor* input_sigma =
        GetInput(context, node, kInputTensorSigma);
    TF_LITE_ENSURE_EQ(context, input_sigma->type, kTfLiteFloat32);
    TF_LITE_ENSURE_EQ(context, NumDimensions(input_sigma), 0);

    TF_LITE_ENSURE_EQ(context, NumOutputs(node), 3);
    TfLiteTensor* output_selected_indices =
        GetOutput(context, node, kSoftNMSOutputTensorSelectedIndices);
    output_selected_indices->type = kTfLiteInt32;
    TfLiteTensor* output_selected_scores =
        GetOutput(context, node, kSoftNMSOutputTensorSelectedScores);
    output_selected_scores->type = kTfLiteFloat32;
    TfLiteTensor* output_num_selected_indices =
        GetOutput(context, node, kSoftNMSOutputTensorNumSelectedIndices);
    output_num_selected_indices->type = kTfLiteInt32;
    SetTensorSizes(context, output_num_selected_indices, {});

    if (is_max_output_size_const) {
      SetTensorSizes(context, output_selected_indices, {max_output_size_value});
      SetTensorSizes(context, output_selected_scores, {max_output_size_value});
    } else {
      SetTensorToDynamic(output_selected_indices);
      SetTensorToDynamic(output_selected_scores);
    }
  } else {
    TF_LITE_ENSURE_EQ(context, NumOutputs(node), 2);
    TfLiteTensor* output_selected_indices =
        GetOutput(context, node, kNMSOutputTensorSelectedIndices);
    output_selected_indices->type = kTfLiteInt32;
    TfLiteTensor* output_num_selected_indices =
        GetOutput(context, node, kNMSOutputTensorNumSelectedIndices);
    output_num_selected_indices->type = kTfLiteInt32;
    SetTensorSizes(context, output_num_selected_indices, {});

    if (is_max_output_size_const) {
      SetTensorSizes(context, output_selected_indices, {max_output_size_value});
    } else {
      SetTensorToDynamic(output_selected_indices);
    }
  }

  return kTfLiteOk;
}